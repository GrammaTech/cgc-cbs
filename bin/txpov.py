#!/usr/bin/env python

import os
import sys
import argparse
import tempfile

import lxml.etree as ET

# TECHx POV helper
# POV format:
# https://github.com/CyberGrandChallenge/cgc-release-documentation/blob/master/pov-markup-spec.txt

# taken from poll-validate
dtd_path = '/usr/share/cgc-docs/replay.dtd'
xml_line = '<?xml version="1.0" standalone="no" ?>'
dtd_line = '<!DOCTYPE pov SYSTEM "%s">' % dtd_path

class TxPOV:

    def __init__(self, tree):
        self.tree = tree

    @classmethod
    def make(cls, cbid):
        root = ET.Element('pov')

        # CBID node
        cbidTag = ET.SubElement(root, 'cbid')
        cbidTag.text = cbid

        # replay tag contains actual POV content
        replayTag = ET.SubElement(root, 'replay')

        tree = ET.ElementTree(root)

        return cls(tree)

    @classmethod
    def make_from_file(cls, filename):
        return cls(ET.parse(filename))

    @classmethod
    def make_from_string(cls, xml_string):
        tree = ET.ElementTree(ET.fromstring(xml_string))
        return cls(tree)

    def add_element(self, elem):
        self.tree.find('replay').append(elem)

    def add_write(self):
        write = ET.SubElement(self.tree.find('replay'), 'write')
        return write

    def add_read(self, length=0):
        read = ET.SubElement(self.tree.find('replay'), 'read')

        # length
        length_elem = ET.SubElement(read, 'length')
        length_elem.text = str(length)

        return read

    @staticmethod
    def add_data(element, data, format):
        dataElem = ET.SubElement(element, 'data', { 'format' : format })
        dataElem.text = data

    @staticmethod
    def add_read_var(element, var, length):
        assign_elem = ET.SubElement(element, 'assign')
        var_elem = ET.SubElement(assign_elem, 'var')
        var_elem.text = var

        slice_elem = ET.SubElement(assign_elem, 'slice', { 'begin' : '0', 'end' : str(length) })

        # update length field
        length_elem = element.find('length')
        length_elem.text = str(length)

    @staticmethod
    def add_write_var(element, var):
        var_elem = ET.SubElement(element, 'var')
        var_elem.text = var

    @staticmethod
    def add_match_data(element, data, format):
        match = ET.SubElement(element, 'match')
        data_elem = ET.SubElement(match, 'data', { 'format' : format })
        data_elem.text = data

        # update length field
        length_elem = element.find('length')
        length = int(length_elem.text)
        divisor = 1
        if format == 'hex':
            divisor = 2
        length += (len(data.decode('string_escape')) / divisor)
        length_elem.text = str(length)

    def get_cbid(self):
        return self.tree.find('cbid').text

    def set_cbid(self, cbid):
        e = self.tree.find('cbid')
        e.text = cbid

    @staticmethod
    def get_length(element):
        length_elem = element.find('length')
        if length_elem != None:
            return int(length_elem.text)
        return None

    def get_next_replay_elem(self):
        r = self.tree.find('replay')
        for e in list(r):
            yield e

    def get_next_write(self):
        r = self.tree.find('replay')
        if r == None:
            yield None

        for w in r.findall('write'):
            yield w

    def remove_last_write(self):
        r = self.tree.find('replay')
        if r == None:
            return False

        last_write = None
        for w in r.findall('write'):
            last_write = w

        if last_write != None:
            r.remove(last_write)
            return True

        return False

    @staticmethod
    def get_text(elem, hex=True):
        s = ''
        for d in elem.findall('data'):
            if 'format' in d.attrib and d.attrib['format'] == 'hex':
                if hex:
                    s += d.text
                else:
                    s += d.text.decode('hex')
            else:
                s += d.text

        return s

    def get_next_line(self, hex=True):
        r = self.tree.find('replay')
        if r == None:
            yield None

        for w in r.findall('write'):
            for d in w.findall('data'):
                if 'format' in d.attrib and d.attrib['format'] == 'hex':
                    if hex:
                        yield d.text
                    else:
                        yield d.text.decode('hex')
                else:
                    yield d.text.decode('string_escape')

    def write(self, filename):
        xml_string = self.to_string()
        with open(filename, 'w') as outfile:
            outfile.write(xml_string)

    def to_string(self, pretty=True):
        xml_str = xml_line + '\n'
        if 'DOCTYPE' not in ET.tostring(self.tree):
            xml_str += dtd_line + '\n'
        xml_str += ET.tostring(self.tree, pretty_print=pretty)
        return xml_str
