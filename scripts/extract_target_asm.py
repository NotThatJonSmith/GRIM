import re
import argparse
import sys
import pathlib

def extract_special_comments(code):
    i = 0
    comments = []
    start_sigil = '/* @EncodeAsm: '
    end_sigil = '*/'
    while i < len(code):

        # Find the start of a special comment
        if code[i:i+len(start_sigil)] != start_sigil:
            i += 1
            continue
        i += len(start_sigil)

        # Get the name of the blob
        end_of_line = code.find('\n', i)
        name = code[i:end_of_line]
        if not name.strip():
            raise ValueError(f'Empty name at line {i}')
        i = end_of_line + 1

        # Find the end of the comment
        end = code.find(end_sigil, i)
        if end == -1:
            raise ValueError(f'Unmatched {start_sigil}')
        yield name, code[i:end]
        i = end + len(end_sigil)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Extract comments from a C++ file.')
    parser.add_argument('-od', '--output_dir', type=pathlib.Path, help='Path to the output directory', default=pathlib.Path('.'))
    parser.add_argument('-n', '--filenames', help='Print the names of the extracted files', action='store_true')
    parser.add_argument('input_files', nargs='+', help='Input files to extract the comments from')
    args = parser.parse_args()
    
    for input_file in args.input_files:
        cpp_code = open(input_file, 'r').read()
        for asm_section in extract_special_comments(cpp_code):
            name, asm_code = asm_section
            name += '.S'
            out_path = args.output_dir / name
            if args.filenames:
                print(out_path.as_posix(), end=' ')
            else:
                print(f'Writing {name} into {out_path}')
                with open(out_path, 'w') as f:
                    f.write(asm_code)
