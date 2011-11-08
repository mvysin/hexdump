// hexdump - Displays binary files in hexidecimal and ASCII format, for Windows.
// Copyright (c) 2011 Michael Vysin
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

static void pw32error(const wchar_t *msg);

int wmain(int argc, wchar_t **argv)
{
    if (argc < 2)
    {
        wprintf(
            L"hexdump - Copyright (c) 2011, Michael Vysin\n"
            L"This program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE\n"
            L"This is free software distributed under the terms of the GNU General Public License\n"
            L"version 3.0 or later, and you are welcome to redistribute it under these conditions.\n\n"

            L"Usage: hd [-a] [-c count] [-d] [-s offset] [-w width] file\n"
            L"Options:\n"
            L"    -a           : display all the data - do not skip duplicate lines\n"
            L"    -c count     : display only [count] bytes from the file\n"
            L"    -d           : double-space the output\n"
            L"    -s offset    : skip [offset] bytes, expressed as a C number literal\n"
            L"    -w width     : use [width] bytes per row, expressed as a C number literal\n\n"
        );
        return 1;
    }

    // default settings
    bool double_space = false;
    bool skip_dups = true;
    unsigned __int64 offset = 0;
    unsigned __int64 count = 0;
    unsigned int width = 0x10;
    const wchar_t *filename = 0;

    // parse command line
    for (int i = 1; i < argc; i++)
    {
        const wchar_t *arg = argv[i];
        if (arg[0] != L'-')
        {
            // not an option - it's the filename
            if (filename != 0)
            {
                fwprintf(stderr, L"hexdump: multiple filenames specified\n");
                return 1;
            }
            filename = arg;
            continue;
        }

        // else... it's an option
        const wchar_t *opt = &arg[1];
        for (const wchar_t *opt = &arg[1]; *opt != 0; opt++)
        {
            if (*opt == L'a')
                skip_dups = false;
            else if (*opt == L'd')
                double_space = true;
            else if (*opt == L'c' || *opt == L's' || *opt == L'w')
            {
                if (*(opt+1) != 0 || i+1 >= argc)      // not at end of argument?
                {
                    fwprintf(stderr, L"hexdump: %c requires parameter\n", *opt);
                    return 1;
                }

                // get value as C literal
                unsigned __int64 value = _wcstoui64(argv[i+1], 0, 0);
                if (*opt == 'c')
                    count = value;
                else if (*opt == 's')
                    offset = value;
                else if (*opt == 'w')
                {
                    if (value == 0)
                    {
                        fwprintf(stderr, L"hexdump: width cannot be zero\n");
                        return 1;
                    }
                    else if (value > UINT_MAX)
                    {
                        fwprintf(stderr, L"hexdump: width too large\n");
                        return 1;
                    }

                    width = (unsigned int)value;
                }

                // skip the parameter we read
                i++;
            }
        }
    }


    //
    // main loop: open the file, read it in chunks, dump it to stdout
    //

    HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        pw32error(L"hexdump: opening file failed");
        return 2;
    }

    // get size
    LARGE_INTEGER w32size;
    if (!GetFileSizeEx(hFile, &w32size))
    {
        pw32error(L"hexdump: unable to determine file size");
        return 2;
    }
    unsigned __int64 size = w32size.QuadPart;


    // and seek to offset
    if (offset > size)
    {
        fwprintf(stderr, L"hexdump: start position past end of file\n");
        return 2;
    }
    else if (offset != 0)
    {
        // interpreted as ULARGE_INTEGER since FILE_BEGIN
        LARGE_INTEGER w32offset;
        w32offset.QuadPart = offset;
        if (!SetFilePointerEx(hFile, w32offset, 0, FILE_BEGIN))
        {
            pw32error(L"hexdump: seek to start position failed");
            return 2;
        }
    }
    

    // our count is equal to either the entire file from offset, or the passed-in value
    count = min(count, size-offset);


    // allocate row buffer
    char *buffer = new char[width];
    if (!buffer)
    {
        fwprintf(stderr, L"hexdump: out of memory: width too large?");
        return 3;
    }

    char *lastbuffer = 0;
    if (skip_dups)
    {
        lastbuffer = new char[width];
        if (!lastbuffer)
        {
            delete [] buffer;
            fwprintf(stderr, L"hexdump: out of memory: width too large?");
            return 3;
        }
    }

    // main loop
    bool print_i64 = (offset+count > UINT_MAX);
    unsigned __int64 position = offset;
    bool in_dup = false;
    for (unsigned __int64 row = 0;; row++)
    {
        DWORD bytesRead = 0;
        DWORD validBytes = 0;
        while (validBytes < width)
        {
            if (!ReadFile(hFile, &buffer[validBytes], width, &bytesRead, 0))
            {
                pw32error(L"hexdump: read failed");
                delete [] buffer;
                if (lastbuffer)
                    delete [] lastbuffer;
                CloseHandle(hFile);
                return 2;
            }

            validBytes += bytesRead;
            if (bytesRead == 0)             // EOF
                break;
        }

        // skip duplicate row?
        if (skip_dups && row > 0 && validBytes == width && !memcmp(buffer, lastbuffer, width))
        {
            if (!in_dup)
            {
                wprintf(L" *** \n");
                if (double_space)
                    wprintf(L"\n");
            }
            in_dup = true;
        }
        else
        {
            in_dup = false;

            // address
            if (print_i64)
                wprintf(L"%016I64x: ", position);
            else
                wprintf(L"%08I64x: ", position);

            // bytes
            for (DWORD i = 0; i < width; i++)
            {
                if ((i % 8) == 0)
                    wprintf(L" ");
                if (i < validBytes)
                    wprintf(L"%02x ", (unsigned int)(unsigned char)buffer[i]);
                else
                    wprintf(L"   ");
            }

            // ascii
            wprintf(L" ");
            for (DWORD i = 0; i < width; i++)
            {
                if (i < validBytes)
                {
                    char ch = buffer[i];
                    if (ch < ' ' || ch > 0x7E)
                        wprintf(L".");
                    else
                        wprintf(L"%c", ch);
                }
                else
                    wprintf(L" ");
            }
        }

        if (!skip_dups || !in_dup)
        {
            wprintf(L"\n");
            if (double_space)
                wprintf(L"\n");
        }

        if (validBytes < width)
            break;

        if (skip_dups && !in_dup)
            memcpy(lastbuffer, buffer, width);
        position += width;
    }


    if (lastbuffer)
        delete [] lastbuffer;
    delete [] buffer;
    CloseHandle(hFile);

    return 0;
}

static void pw32error(const wchar_t *msg)
{
    fwprintf(stderr, L"%s: ", msg);

    wchar_t *w32msg = 0;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
        0, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (wchar_t*)&w32msg, 0, 0);
    if (w32msg != 0)
    {
        fwprintf(stderr, L"%s\n", w32msg);
        LocalFree(w32msg); 
    }
    else
        fwprintf(stderr, L"(Unknown Error)\n");
}