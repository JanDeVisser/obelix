/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string>

#include <oblasm/Assembler.h>

namespace Obelix::Assembler {

int assemble(std::string const& in_file, std::string const& out_file)
{
    Image image;

    FileBuffer buffer(in_file);
    if (!buffer.file_is_read()) {
        fprintf(stderr, "Could not open input file '%s': %s\n", in_file.c_str(), buffer.error().message().c_str());
        return -1;
    }
    Assembler assembler(image);
    assembler.parse(buffer.buffer()->str());
    if (image.assemble().empty()) {
        for (auto& err : image.errors()) {
            fprintf(stderr, "%s\n", err.c_str());
        }
        return -1;
    }
    if (auto size_maybe = image.write(out_file); size_maybe.is_error()) {
        fprintf(stderr, "Could not write output file '%s': %s\n", out_file.c_str(), size_maybe.error().message().c_str());
        return -1;
    } else {
        fprintf(stderr, "Success. Wrote %d bytes to %s\n", size_maybe.value(), out_file.c_str());
    }
    return 0;
}

}

void usage()
{
    fprintf(stderr, "Usage: jv80asm [-o outfile] infile\n");
    exit(1);
}

int main(int argc, char** argv)
{
    std::string out_file = "out.bin";
    std::string in_file;

    auto ix = 1;
    while (ix < argc) {
        if (!strcmp(argv[ix], "-o")) {
            if (++ix < argc) {
                out_file = argv[ix++];
                continue;
            }
            usage();
        }
        in_file = argv[ix++];
    }
    if (in_file.empty())
        usage();

    return Obelix::Assembler::assemble(in_file, out_file);
}
