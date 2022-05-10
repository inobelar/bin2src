# Bin2src

## Build

- `$ bash ./build.sh`

## Usage

- `$ ./bin2src -i input_file_name -o output_file_name -n variable_name [-m mode]`
    - Example (convert itself to bytes, produces `resource_bin2src.h` and `resource_bin2src.c`):
        - `$ ./bin2src -i ./bin2src -o resource_bin2src -n bin2src -m c_funcs`

## Depencies

- [GitHub :: jibsen/parg](https://github.com/jibsen/parg) - library for portable arguments parsing in C.

## Alternatives

- [xxd](https://linux.die.net/man/1/xxd)
    - [Usage]((https://unix.stackexchange.com/a/176112)): `$ xxd --include filename > resource_filename.h`
- [GitHub :: gwilymk/bin2c](https://github.com/gwilymk/bin2c)
    - Generates single C header file from single file
    - Features: supports bzip2 compression
- [GitHub ::  Jeroen6/bin2c](https://github.com/Jeroen6/bin2c)
    - Generates headers C and sources from multiple files as input
- [GitHub :: adobe/bin2c](https://github.com/adobe/bin2c)
