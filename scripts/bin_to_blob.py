import argparse
from pathlib import Path

def generate_header(bin_path, header_path=None):
    file_stem = bin_path.stem
    variable_name = file_stem + "_bytes"
    variable_name = variable_name.replace(".", "_")
    if not header_path:
        header_path = file_stem + ".h"
    header_content = f"#pragma once\n"
    header_content += f"const unsigned char {variable_name}[] = {{" 
    with open(bin_path, 'rb') as bin_file:
        for byte in bin_file.read():
            header_content += f"0x{byte:02X}, "
    header_content = header_content[:-2] + "};\n"
    with open(header_path, 'w') as output_file:
        output_file.write(header_content)

def main():
    parser = argparse.ArgumentParser(description="Generate a C++ header file from a binary file.")
    parser.add_argument("input_file", type=Path, help="Input binary file path")
    parser.add_argument("--output-path", "-o", type=Path, default=None, help="Output header file path")
    args = parser.parse_args()
    generate_header(args.input_file, args.output_path)

if __name__ == "__main__":
    main()
