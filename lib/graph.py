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

import os
import random
import matplotlib
import sys
if 'matplotlib.backends' not in sys.modules:
    matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.font_manager as fmt


class Graph(object):
    """ Graph -  Throw - Perform the interactions with a CB

    This class implements the graph implementation and traversal functionality
    of generate-poll.

    Usage:
        a = Graph()
        a.add_node('foo')
        a.add_node('bar')
        a.add_edge('foo', 'bar')
        a.walk()

    Attributes:
        max_depth: Maximum number of edges to traverse 
        start: Which node to start traversal of the graph
    """
    def __init__(self):
        self._depth = 0
        self._neighbors = {}
        self._nodes = {}
        self.max_depth = 0xFFFFF
        self.start = None

    def add_node(self, node, chance=1.0, continue_chance=1.0):
        """ Add a node to the graph

        Args:
            node: a callable method that implements the function for the node
            chance: A float that specifies the likelyhood that the state
                machine will execute the node.  Values should be greater than
                0.0 and less than or equal to 1.0
            continue_chance: A float that specifies the likelyhood that the
                state machine will continue execution upon completing the
                node's execution.  Values should be greater than 0.0 and less
                than or equal to 1.0

        Returns:
            None

        Raises:
            Exception if chance is not a float
            Exception if chance not greater than 0.0 and less than or equal to 1.0
            Exception if continue_chance is not a float
            Exception if continue_chance not greater than 0.0 and less than or equal to 1.0
            Exception if node is not callable
            Exception if node has already been defined in the graph
        """
        assert isinstance(chance, float)
        assert chance > 0.0 and chance <= 1.0
        assert isinstance(continue_chance, float)
        assert continue_chance > 0.0 and continue_chance <= 1.0
        assert callable(node)
        assert node not in self._nodes

        self._nodes[node] = {'chance': chance, 'continue': continue_chance, 'depth': []}

    def add_edge(self, node1, node2, weight=1):
        """ Add a weighted edge from one node to another node

        Args:
            node1: The parent node
            node2: The child node
            weight: The weighting of the the edge between node1 and node2

        Returns:
            None

        Raises:
            Exception if node1 or node2 are not defined nodes
            Exception if weight is not a float or integer
            Exception if an edge from node1 to node2 already exists
        """
        assert node1 in self._nodes
        assert node2 in self._nodes
        assert isinstance(weight, (int, float))

        if node1 not in self._neighbors:
            self._neighbors[node1] = {}

        assert node2 not in self._neighbors[node1]
        self._neighbors[node1][node2] = {'weight': weight, 'depth': []}

    def _random_edge(self, node):
        """ Select a random edge to traverse from a given node, based on the
        weighted edges defined during 'add_edge'

        Args:
            node: The node to select edges from

        Returns:
            A random node based on the weighted values

        Raises:
            None
        """
        nodes = self._neighbors[node].keys()
        nodes.sort()
        weights = []
        for edge_node in nodes:
            weights.append(self._neighbors[node][edge_node]['weight'])
        value = random.random() * sum(weights)
        for i, weight in enumerate(weights):
            value -= weight
            if value <= 0:
                return nodes[i]

    def walk(self):
        """ Walk the directed graph

        Starting at a random node, or the 'start' node if specified,
        iteratively walk the directed graph.  At each node, execute it if the
        node's 'chance' value is successful.  

        The graph traversal will continue until a node is executed that have no
        edges from it, the method implementing the node returns -1, the node's
        continue value is not successful, or if more than the specified
        max_depth traversals have occured.

        Args:
            None

        Returns:
            None

        Raises:
            None
        """
        node = self.start
        if node is None:
            node = random.choice(self._nodes.keys())
        assert node in self._nodes

        prev = None
        while True:
            if random.random() < self._nodes[node]['chance']:
                if prev is not None:
                    self._neighbors[prev][node]['depth'].append(self._depth)

                self._depth += 1

                self._nodes[node]['depth'].append(self._depth)
                response = node()
                if response == -1:
                    break

            if random.random() > self._nodes[node]['continue']:
                break

            if node not in self._neighbors:
                break
            
            if self._depth > self.max_depth:
                break

            prev = node
            node = self._random_edge(node)

        self._depth = 0

    @classmethod
    def _make_plot(cls, title, depths, names, filename):
        """ Create a PNG plot of the graph data

        Args:
            title: title of the plot
            depths: Values to be graphed
            names: Names of each of the bins being graphed
            filename: Full path of the file for the resulting PNG

        Returns:
            None

        Raises:
            None
        """
        plt.figure()
        plt.title(title)
        plt.xlabel('Graph Depth')
        plt.ylabel('Count')
        plt.margins(0.01)
        plt.subplots_adjust(bottom=0.15)
        plt.hist(depths, histtype='bar', label=names, bins=10)
        plt.rc('legend', **{'fontsize': 8})
        plt.legend(shadow=True, fancybox=True)
        plt.savefig(filename, format='png')

    def plot(self, directory):
        """ Create plots of data from the graph traversals that have occured thus far.

        Creates a plot showing the execution of nodes per each depth of the
        traversal, and a plot showing the edge traversals per depth of the traversal.

        Args:
            directory: Directory to write the resulting plots

        Returns:
            None

        Raises:
            AssertionError if a node was never executed
            AssertionError if an edge was never traversed
        """
        depths = []
        names = []
        for node in self._nodes:
            assert len(self._nodes[node]['depth']), "node %s was never executed" % node.func_name
            depths.append(self._nodes[node]['depth'])
            names.append(node.func_name)

        self._make_plot('Node execution per depth', depths, names,
                        os.path.join(directory, 'nodes.png'))

        depths = []
        names = []
        for node in self._neighbors:
            for sub_node in self._neighbors[node]:
                assert len(self._neighbors[node][sub_node]['depth']), "Edge %s->%s was not traversed" % (node.func_name, sub_node.func_name)
                depths.append(self._neighbors[node][sub_node]['depth'])
                names.append('%s->%s' % (node.func_name, sub_node.func_name))

        self._make_plot('Edge traversal per depth', depths, names,
                        os.path.join(directory, 'edges.png'))

    def dot(self):
        """ Return the existing graph into DOT format

        Args:
            None

        Returns:
            String containing the graph in DOT format

        Raises:
            None
        """
        text = []
        text.append('digraph G {')
        for node in self._nodes:
            notes = [node.func_name]

            if self._nodes[node]['chance'] != 1.0:
                notes.append('chance=%.2f' % self._nodes[node]['chance'])

            if self._nodes[node]['continue'] != 1.0:
                notes.append('continue=%.2f' % self._nodes[node]['continue'])

            shape = ''
            if self.start == node:
                shape = ', shape=box'
            if node not in self._neighbors:
                shape = ', shape=triangle'
            text.append('    %s [label="%s"%s];' % (node.func_name,
                                                    '\\n'.join(notes), shape))
        for node in self._neighbors:
            for sub_node in self._neighbors[node]:
                sub_text = ''
                if self._neighbors[node][sub_node]['weight'] != 1:
                    label = str(self._neighbors[node][sub_node]['weight'])
                    sub_text = ' [label="%sx"]' % label
                text.append('    %s -> %s%s;' % (node.func_name,
                                                 sub_node.func_name, sub_text))
        text.append('}')
        return '\n'.join(text)
