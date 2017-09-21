#!/usr/bin/env python

import sys
import argparse
import glob
from txpov import *

def xml_to_file(xml_name, out_name):
    with open(out_name, 'w') as out_file:
        pov = TxPOV.make_from_file(xml_name)

        for line in pov.get_next_line(hex=False):
            out_file.write(line)

    return os.path.isfile(out_name)

if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument("-x", "--xml", help="xml input file(s)", 
                        type=str, required=True, nargs='*')
    parser.add_argument("-o", "--outname", help="output file name",
                        type=str, required=False)

    args = parser.parse_args()

    for file in args.xml:
        if not os.path.isfile(file):
            sys.stderr.write("File '%s' not found. Skipping.\n" % file)
            continue

        if args.outname != None:
            out_name = args.outname
        else:
            # strip .xml extension to produce output name
            out_name = os.path.splitext(file)[0]

        xml_to_file(file, out_name)
