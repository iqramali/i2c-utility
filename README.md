

## iQ-i2c util program for Linux systems to use i2c commands in ARM devices.

In this utility you can read the i2c device registers in raw binary output and feed the write data via file.

# Prerequistes to compile this program
1. Linux machine, 
2. Cross-compiler iQ-i2c.c with GCC compiler


# This is the complete helper functions available in iQ-i2c utility

		"Welcome to iQ-i2c helper utility"
		" Usages:"
		" Read register:"
		"   %s [-opts] read <bus> <device> <register> <length> [raw]"
		"     'raw' will dump the data as is, without this"
		"     option it will be printed in hex."
		"
		" Write register:
		"   %s [-opts] write <bus> <device> <register|-> "
		"{<path>|<hexbytes>}"
		"
		"
		"  <bus>      - i2c-bus id 1-4"
		"  <device>   - i2c device address in hex"
		"  <register> - i2c device address in hex (any length)"
		"               if specified as '-' the register is ignored."
		"               same format as <hexbytes>"
		"  <length>   - length in bytes to read"
		"  raw        - print data as is, else print as hex"
		"  <path>     - file path with binary data"
		"  @<path>    - file path with hex data"
		"  <hexbytes> - In the format hh x N, i.e:"
		"               c1ffc4d3abed02"
		"
		"  [-opts] options:"
		"     -v      - verbose"
		"     -r      - raw binary output (not hex)"
		"     -f      - force device access even if locked by driver"
  
# How to read register of certain i2c device?
You can read as mentioned below

		"   %s [-opts] read <bus> <device> <register> <length> [raw]"
		"     'raw' will dump the data as is, without this"
		"     option it will be printed in hex"
    
 # How to write register of certain i2c device?
 You can feed the file by providing its path
 
 		" Write register:"
		"   %s [-opts] write <bus> <device> <register|-> "
		"{<path>|<hexbytes>}"
    
 
