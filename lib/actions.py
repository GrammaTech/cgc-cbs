#!/usr/bin/env python
#
# Copyright (C) 2014 Brian Caswell <bmc@lungetech.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import random
import string


class Actions(object):
    """Actions - Define the interactions for a CB

    This class implements the basic methods to interact with a CB, in terms of
    XML generation for use with 'cb-replay'.

    Usage:
        a = Actions():
        a.write('foo')
        a.read(delim='\n')
        a.xml()

    Attributes:
        state: Dict of state values, to be reset upon each iteration.
    """

    def __init__(self):
        self._actions = []
        self.state = {}

    @classmethod
    def chance(cls, value=0.5):
        """ Randomly return True or False, with the likelyhood defined by
        specifying a percentage argument.

        Args:
            value: A float between 0.0 and 1.0

        Returns:
            True or False

        Raises:
            Exception: if 'value' is not a float between 0.0 and 1.0
        """
        assert isinstance(value, float)
        assert value > 0.0 and value < 1.0
        return random.random() < value

    def reset(self):
        """ Re-initialize the instance of the class
        
        Args:
            None

        Returns:
           None

        Raises:
            None
        """
        self.__init__()

    def xml(self):
        """ Returns the XML of the existing actions following the replay.dtd spec 

        Args:
            None

        Returns:
           String containing XML of the existing actions

        Raises:
            None
        """
        lines = [
            '<?xml version="1.0" standalone="no" ?>',
            '<!DOCTYPE pov SYSTEM "/usr/share/cgc-replay/replay.dtd">'
        ]
        out = '\n'
        actions = '\n    ' + '\n    '.join(self._actions) + '\n'
        out += self._wrap('cbid', 'service') + '\n'
        out += self._wrap('replay', actions) + '\n'
        lines.append(self._wrap('pov', out))
        return '\n'.join(lines)

    @classmethod
    def _wrap(cls, tag, value, **options):
        """ Creates an XML eliment 

        Args:
            tag: The tag name
            value: The value of the tag
            **options: arbitrary named arguments, used as attributes for the tag

        Returns:
           String containing XML element

        Raises:
            None
        """
        opt_string = ''

        if len(options):
            opts = []
            for key in options:
                opts.append('%s="%s"' % (key, options[key]))
            opt_string = ' ' + ' '.join(opts)
        return '<%s%s>%s</%s>' % (tag, opt_string, value, tag)

    @classmethod
    def _encode(cls, data):
        """ Encodes a string to the 'cstring' encoding supported by the replay DTD.

        Args:
            data: string value to be encoded

        Returns:
           String containing the encoded value

        Raises:
            None
        """
        chars = string.letters + string.digits + " ?!:."
        return ''.join([x if x in chars else "\\x%02x" % ord(x) for x in data])

    def read(self, delim=None, length=None, expect=None, expect_format=None,
             timeout=None):
        """ Create a 'read' interaction for a challenge binary as supported by the replay DTD.

        Args:
            delim: Specify data should be read until the string has been received from the CB
            length: Specify the amount of data to be read from the CB.
            expect: Specify the expected value that should be returned from the CB
            expect_format: Specify the format of the 'expect' value, allowed
                values are 'pcre' or 'asciic'.  Unless a value is specified,
                the default format is 'asciic'.

        Returns:
           None

        Raises:
            Exception if delim AND length are specified
            Exception if expect_format is specified and expect is not specified
            Exception if expect_format is not 'pcre' or 'asciic'
            Exception if timeout is not an integer
            Exception if length is not an integer
            Exception if the delimiter is not a string
            Exception if the delimiter is an empty string
        """
        assert length is not None or delim is not None
        if expect_format is not None:
            assert expect is not None
            assert expect_format in ['pcre', 'asciic']

        xml = ''

        if timeout is not None:
            assert isinstance(timeout, int)
            xml += self._wrap('timeout', str(timeout))

        if length is not None:
            assert isinstance(length, int)
            assert length > 0
            xml += self._wrap('length', str(length))

        if delim is not None:
            assert isinstance(delim, str)
            assert len(delim) > 0
            delim = self._encode(delim)
            xml += self._wrap('delim', delim)

        if expect is not None:
            match = None
            if expect_format == 'pcre':
                match = self._wrap('pcre', expect)
            else:
                match = self._wrap('data', self._encode(expect))
            xml += self._wrap('match', match)

        xml = self._wrap('read', xml)
        self._actions.append(xml)

    def comment(self, msg, *args):
        """ Create an XML comment of 'msg % args'

        Args:
            msg: Message to be logged
            args: Arguments to be interpreted via str formatting
        
        Returns
            None 

        Raises:
            None 
        """
        data = '<!-- %s -->' % self._encode(msg % args)
        self._actions.append(data)

    def write(self, data):
        """ Create a 'read' interaction for a challenge binary as supported by the replay DTD.

        Args:
            data: Specify the data that should be sent to the CB.
        
        Returns
            None 

        Raises:
            None 
        """
        data = self._encode(data)
        self._actions.append(self._wrap('write', self._wrap('data', data)))
