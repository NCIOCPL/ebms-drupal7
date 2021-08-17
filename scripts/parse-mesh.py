#!/usr/bin/env python3

"""Extract hierarchy information for publication types from MeSH tree.

This is the URL for downloading MeSH, which is used as the input:
https://nlmpubs.nlm.nih.gov/projects/mesh/MESH_FILES/xmlmesh/desc2021.gz
(they don't have a "current" or "latest" alias, so the year has to be
changed as appropriate).

The contents of the resulting pubtype-ancestors.json file is then put
into the ebms_config table under the name "pubtype-ancestors" and used
by the software which assembled the article review pages in order to
select which publication types to display for the articles, using the
logic laid out in the requirements for the EBMS Denali release ticket
https://tracker.nci.nih.gov/browse/OCEEBMS-593.
"""

from argparse import ArgumentParser
from gzip import decompress
from json import dump
from lxml import etree

class Descriptor:
    def __init__(self, node):
        self.__node = node
    @property
    def name(self):
        if not hasattr(self, "_name"):
            self._name = self.__node.find("DescriptorName/String").text
        return self._name
    @property
    def tree_numbers(self):
        if not hasattr(self, "_tree_numbers"):
            self._tree_numbers = []
            for node in self.__node.findall("TreeNumberList/TreeNumber"):
                self._tree_numbers.append(node.text)
        return self._tree_numbers


parser = ArgumentParser()
parser.add_argument("--descriptors", "-d", default="desc2021.gz")
opts = parser.parse_args()
with open(opts.descriptors, "rb") as fp:
    root = etree.fromstring(decompress(fp.read()))
numbers = {}
names = {}
for node in root.xpath('DescriptorRecord[@DescriptorClass="2"]'):
    descriptor = Descriptor(node)
    for tree_number in descriptor.tree_numbers:
        if tree_number in numbers:
            name = descriptor.name
            other = numbers[tree_number]
            message = f"{tree_number} appears twice: {name} and {other}"
            raise Exception(message)
        name = descriptor.name.strip().lower()
        numbers[tree_number] = name
        if name not in names:
            names[name] = [tree_number]
        else:
            names[name].append(tree_number)
values = {}
for name in sorted(names):
    ancestors = set()
    for tree_number in names[name]:
        pieces = tree_number.split(".")
        depth = 1
        while depth < len(pieces):
            ancestor = ".".join(pieces[:depth])
            ancestors.add(numbers[ancestor])
            depth += 1
    values[name] = sorted(ancestors)
with open("pubtype-ancestors.json", "w") as fp:
    dump(values, fp, indent=2)
