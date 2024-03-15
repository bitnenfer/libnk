import sys
import os

def convert_to_c_header(input_files, output_filename):
    try:
        # Extract header guard name from the output file name
        header_guard_name = os.path.splitext(os.path.basename(output_filename))[0].upper()
        header_guard_name = f"_{header_guard_name}_"
        header_guard_name = header_guard_name.replace(".", "_")  # Replace dots with underscores

        # Write C header file with hex values
        with open(output_filename, 'w') as output_file:
            output_file.write(f"#ifndef {header_guard_name}\n")
            output_file.write(f"#define {header_guard_name}\n\n")

            for input_file_name in input_files:
                # Read content from the input file
                with open(input_file_name, 'rb') as input_file:
                    data = input_file.read()

                # Extract variable name without extension
                variable_name = os.path.splitext(os.path.basename(input_file_name))[0]

                # Write hex values in 12 columns for each file
                output_file.write(f"unsigned char {variable_name}[] = {{\n")
                for i, byte in enumerate(data):
                    if i % 12 == 0:
                        output_file.write("    ")
                    output_file.write(f"0x{byte:02X}, ")
                    if (i + 1) % 12 == 0:
                        output_file.write("\n")
                output_file.write("};\n\n")
                output_file.write(f"unsigned int {variable_name}_size = {len(data)};\n\n")

            output_file.write(f"#endif // {header_guard_name}\n")

        print(f"Conversion successful. Output file: {output_filename}")

    except FileNotFoundError:
        print(f"Error: One or more input files not found.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    # Check if correct number of command line arguments is provided
    if len(sys.argv) < 4 or sys.argv[-2] != "-o":
        print("Usage: python bin2c.py input_file1 input_file2 ... -o output_file")
        sys.exit(1)

    # Extract input file names and output file name
    input_files = sys.argv[1:-2]
    output_file_name = sys.argv[-1]

    convert_to_c_header(input_files, output_file_name)
