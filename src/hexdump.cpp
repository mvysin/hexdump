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

static void usage();

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        return 1;
    }



    return 0;
}


static void usage()
{
    printf(
        "hexdump - Copyright (c) 2011, Michael Vysin\n"
        "This program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE\n"
        "This is free software distributed under the terms of the GNU General Public License\n"
        "version 3.0 or later, and you are welcome to redistribute it under these conditions.\n\n"

        "Usage: hd [-a] [-d] [-s offset] [-w width] file\n"
        "Options:\n"
        "    -a           : display all the data - do not skip duplicate lines\n"
        "    -d           : double-space the output\n"
        "    -s offset    : skip [offset] bytes, expressed as a C number literal\n"
        "    -w width     : use [width] bytes per row, expressed as a C number literal\n\n"
    );
}