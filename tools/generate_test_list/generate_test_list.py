#!/usr/bin/env python3
#
# Copygright (c) 2023 Google LLC. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""Generates the same output as generate_test_list.pl, but using python.

Takes an xml file describing a list of testsuites as well as a skip list file
and outputs a src and header file that refers to those tests.
"""

from xml.etree.ElementTree import Element, SubElement, TreeBuilder
import argparse
import os.path
import urllib.parse
import xml.etree.ElementInclude
import xml.parsers.expat

XINCLUDE_INCLUDE = 'xi:include'

MAX_EXPANSION_DEPTH = 5

# Intermediate repesentation classes.


class Testcase:
  """Class representing a single TFTF test case."""

  def __init__(self, name, function, description):
    self.name = name
    self.function = function
    self.description = description if description else ''

  def __str__(self):
    return 'Testcase: name = {}, function = {}, description = {}'.format(
        self.name, self.function, self.description
    )


class Testsuite:
  """Class representing a single TFTF test suite."""

  def __init__(self, name, description, testcases):
    self.name = name
    self.description = description
    self.testcases = testcases

  def __str__(self):
    testcases_strs = [str(tc) for tc in self.testcases]
    return (
        'Testsuite: name = {}, description = {}\n  '.format(
            self.name, self.description
        )
        + '\n  '.join(testcases_strs)
        + '\n'
    )


def FindElementWithNameOrReturnNone(iterable, name):
  """Looks through iterable for an element whose 'name' field matches name."""
  return next(filter(lambda x: x.name == name, iterable), None)


def ParseTestsuitesElementIntoIR(root):
  """Given the root of a parsed XML file, construct Testsuite objects."""
  testsuite_xml_elements = root.findall('.//testsuite')

  testsuites = []
  # Parse into IR
  for testsuite in testsuite_xml_elements:
    testcases = []
    for testcase in testsuite.findall('testcase'):
      testcases += [
          Testcase(
              testcase.get('name'),
              testcase.get('function'),
              testcase.get('description'),
          )
      ]
    testsuites += [
        Testsuite(
            testsuite.get('name'), testsuite.get('description'), testcases
        )
    ]

  return testsuites


# In order to keep this script standalone (meaning no libraries outside of the
# standard library), we have to do our own assembling of the XML Elements. This
# is necessary because python doesn't give us a nice way to support external
# entity expansion. As such we have to use the low level expat parser and build
# the tree using TreeBuilder.


def ParseXmlNoXIncludeExpansion(filename):
  """Parse filename into an ElementTree.Element, following external entities."""
  xml_dir_root = os.path.dirname(filename)
  with open(filename) as fobj:
    xml_contents = fobj.read()

  parser = xml.parsers.expat.ParserCreate()
  parser.SetParamEntityParsing(
      xml.parsers.expat.XML_PARAM_ENTITY_PARSING_ALWAYS
  )

  global treebuilder
  treebuilder = TreeBuilder()
  global expansion_depth
  expansion_depth = 0

  def StartElementHandler(name, attributes):
    # ElementInclude.include requires that the XInclude namespace is expanded.
    if name == 'xi:include':
      name = '{http://www.w3.org/2001/XInclude}include'
    treebuilder.start(name, attributes)

  def EndElementHandler(name):
    treebuilder.end(name)

  def ExternalEntityRefHandler(context, base, systemId, publicId):
    global expansion_depth

    external_entity_parser = parser.ExternalEntityParserCreate(context, 'utf-8')
    AssignAllParserCallbacks(external_entity_parser)
    with open(os.path.join(xml_dir_root, systemId)) as fobj:
      sub_xml_contents = fobj.read()
      expansion_depth += 1
      if expansion_depth > MAX_EXPANSION_DEPTH:
        raise ValueError('Max entity expansion depth reached')

      external_entity_parser.Parse(sub_xml_contents, True)
      expansion_depth -= 1
    return 1

  def AssignAllParserCallbacks(p):
    p.StartElementHandler = StartElementHandler
    p.EndElementHandler = EndElementHandler
    p.ExternalEntityRefHandler = ExternalEntityRefHandler

  AssignAllParserCallbacks(parser)
  parser.Parse(xml_contents, True)

  return treebuilder.close()


# Older versions of python3 don't support ElementInclude.include's base_url
# kwarg. This callable class works around this.
# base_url allows XInclude paths relative to the toplevel XML file to be used.
class ElementIncludeLoaderAdapter:
  """Adapts between ElementInclude's loader interface and our XML parser."""

  def __init__(self, base_url):
    self.base_url = base_url

  def __call__(self, href, parse):
    if parse != 'xml':
      raise ValueError("'parse' must be 'xml'")

    return ParseXmlNoXIncludeExpansion(
        urllib.parse.urljoin(self.base_url, href)
    )


def ParseTestsuitesFromFile(filename):
  """Given an XML file, parse the contents into a List[Testsuite]."""
  root = ParseXmlNoXIncludeExpansion(filename)

  base_url = os.path.abspath(filename)
  loader = ElementIncludeLoaderAdapter(base_url)
  xml.etree.ElementInclude.include(root, loader=loader)

  if root.tag == 'testsuites':
    testsuites_xml_elements = [root]
  elif root.tag == 'document':
    testsuites_xml_elements = root.findall('testsuites')
  else:
    raise ValueError(
        "Unexpected root tag '{}' in {}".format(root.tag, filename)
    )

  testsuites = []

  for testsuites_xml_element in testsuites_xml_elements:
    testsuites += ParseTestsuitesElementIntoIR(testsuites_xml_element)

  return testsuites


def RemoveSkippedTests(testsuites, skip_tests_filename):
  """Remove skipped tests from testsuites based on skip_tests_filename."""
  with open(skip_tests_filename) as skipped_file:
    skipped_file_lines = skipped_file.readlines()
    for i, l in enumerate(skipped_file_lines):
      line = l.strip()

      # Skip empty lines
      if not line:
        continue

      # Skip comments
      if line[0] == '#':
        continue

      testsuite_name, sep, testcase_name = line.partition('/')

      testsuite = FindElementWithNameOrReturnNone(testsuites, testsuite_name)

      if not testsuite:
        print(
            "WARNING: {}:{}: Test suite '{}' doesn't exist or has already "
            'been deleted.'.format(skip_tests_filename, i + 1, testsuite_name)
        )
        continue

      if not testcase_name:
        print("INFO: Testsuite '{}' will be skipped".format(testsuite_name))
        testsuites = list(
            filter(lambda x: x.name != testsuite_name, testsuites)
        )
        continue

      testcase = FindElementWithNameOrReturnNone(
          testsuite.testcases, testcase_name
      )
      if not testcase:
        print(
            "WARNING: {}:{}: Test case '{}/{} doesn't exist or has already "
            'been deleted'.format(
                skip_tests_filename, i + 1, testsuite_name, testcase_name
            )
        )
        continue

      print(
          "INFO: Testcase '{}/{}' will be skipped.".format(
              testsuite_name, testcase_name
          )
      )
      testsuite.testcases.remove(testcase)

    return testsuites


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument(
      'testlist_src_filename', type=str, help='Output source filename'
  )
  parser.add_argument(
      'testlist_hdr_filename', type=str, help='Output header filename'
  )
  parser.add_argument('xml_test_filename', type=str, help='Input xml filename')
  parser.add_argument(
      '--plat_skipped_list_filename',
      type=str,
      help='Filename containing tests to skip for this platform',
      required=False,
  )
  parser.add_argument(
      '--arch_skipped_list_filename',
      type=str,
      help='Filename containing tests to skip for this architecture',
      required=False,
  )
  args = parser.parse_args()

  testsuites = ParseTestsuitesFromFile(args.xml_test_filename)

  # Check validity of names
  testsuite_name_set = set()
  for ts in testsuites:
    if '/' in ts.name:
      raise ValueError(
          'ERROR: {}: Invalid test suite name {}'.format(
              args.xml_test_filename, ts.name
          )
      )

    if ts.name in testsuite_name_set:
      raise ValueError(
          "ERROR: {}: Can't have 2 test suites named {}".format(
              args.xml_test_filename, ts.name
          )
      )

    testsuite_name_set.add(ts.name)

    testcase_name_set = set()
    for tc in ts.testcases:
      if tc.name in testcase_name_set:
        raise ValueError(
            "ERROR: {}: Can't have 2 tests named {}".format(
                args.xml_test_filename, testcase_name
            )
        )

      testcase_name_set.add(tc.name)

  if args.plat_skipped_list_filename:
    testsuites = RemoveSkippedTests(testsuites, args.plat_skipped_list_filename)

  if args.arch_skipped_list_filename:
    testsuites = RemoveSkippedTests(testsuites, args.arch_skipped_list_filename)

  # Flatten all testcases
  combined_testcases = [tc for ts in testsuites for tc in ts.testcases]

  # Generate header file
  with open(args.testlist_hdr_filename, 'w') as hdr_file:
    file_hdr_contents = """#ifndef __TEST_LIST_H__
#define __TEST_LIST_H__

#define TESTCASE_RESULT_COUNT {}

#endif  // __TEST_LIST_H__
""".format(len(combined_testcases))
    hdr_file.write(file_hdr_contents)

  with open(args.testlist_src_filename, 'w') as src_file:
    src_file.write('#include "tftf.h"\n\n')
    # Generate test function prototypes.
    for t in combined_testcases:
      src_file.write('test_result_t {}(void);\n'.format(t.function))

    testcase_index = 0
    # Generate testcase lists.
    for i, testsuite in enumerate(testsuites):
      src_file.write('\nconst test_case_t testcases_{}[] = {{\n'.format(i))
      for testcase in testsuite.testcases:
        src_file.write(
            '  {{ {}, "{}", "{}", {} }},\n'.format(
                testcase_index,
                testcase.name,
                testcase.description,
                testcase.function,
            )
        )
        testcase_index += 1

      src_file.write('  { 0, NULL, NULL, NULL }\n')
      src_file.write('};\n\n')

    # Generate testsuites list.
    src_file.write('const test_suite_t testsuites[] = {\n')
    for i, testsuite in enumerate(testsuites):
      src_file.write(
          '  {{ "{}", "{}", testcases_{} }},\n'.format(
              testsuite.name, testsuite.description, i
          )
      )

    src_file.write('  { NULL, NULL, NULL }\n')
    src_file.write('};\n')
