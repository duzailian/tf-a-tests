# !/usr/bin/env python
# 
# Copyright (c) 2024 Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#


#python3 generate_smc.py -s smclist
#This script generated C files after the read of the SMC description/list file
import re
import argparse
import copy
import sys

parser = argparse.ArgumentParser(
		prog='generate_smc.py',
		description='Generates SMC code to add to fuzzer library',
		epilog='one argument input')

parser.add_argument('-s', '--smclist',help="SMC list file .")

args = parser.parse_args()

print("starting generate SMC")

smclistfile =  open(args.smclist, "r")
smclist_lines = smclistfile.readlines()
smclistfile.close()
seq = 0
arglst = {}
argnumfield = {}
argfieldname = {}
argstartbit = {}
argendbit = {}
argdefval = {}
smcname = ""
argnum = ""
argname = ""
for sline in smclist_lines:
	lcon = 0
	sl = sline.strip()
	sinstr = re.search(r'^smc:\s*([a-zA-Z0-9_]+)$',sl)
	if sinstr:
		smcname = sinstr.group(1)
		arglst[sinstr.group(1)] = []
		argnumfield[sinstr.group(1)] = {}
		argfieldname[sinstr.group(1)] = {}
		argstartbit[sinstr.group(1)] = {}
		argendbit[sinstr.group(1)] = {}
		argdefval[sinstr.group(1)] = {}
		lcon = 1
		argoccupy = {}
		if not seq:
			seq = seq + 1
		else:
			if seq != 2:
				print("Error: out of sequence for smc call",end=" ")
				print(smcname)
				sys.exit()
			else:
				seq = 1
	sinstr = re.search(r'^arg(\d+)\s*:\s*([a-zA-Z0-9_]+)$',sl)
	if sinstr:
		if sinstr.group(1) in argoccupy:
			print("Error: register already specified for SMC call",end=" ")
			print(smcname,end=" ")
			print("argument",end=" ")
			print(sinstr.group(1))
			sys.exit()
		argnum = sinstr.group(1)
		argname = sinstr.group(2)
		arglst[smcname].append(argnum)
		argnumfield[smcname][sinstr.group(2)] = {}
		argfieldname[smcname][sinstr.group(2)] = []
		argstartbit[smcname][sinstr.group(2)] = {}
		argendbit[smcname][sinstr.group(2)] = {}
		argdefval[smcname][sinstr.group(2)] = {}
		lcon = 1
		argoccupy[argnum] = 1
		fieldoccupy = []
		if seq != 1:
			if seq != 2:
				print("Error: out of sequence for arg(value)",end=" ")
				print("arg",end=" ")
				print(argnum,end=" ")
				print("for argname",end=" ")
				print(argname)
				sys.exit()
			else:
				seq = 2
		else:
			seq = seq + 1

	sinstr = re.search(r'^arg(\d+)\s*=\s*(0x[a-fA-F0-9]+|\d+)$',sl)
	if sinstr:
		if sinstr.group(1) in argoccupy:
			print("Error: register already specified for SMC call",end=" ")
			print(smcname,end=" ")
			print("argument",end=" ")
			print(sinstr.group(1))
			sys.exit()
		srange = sinstr.group(1)
		argrangename = smcname + "_args_"+ sinstr.group(1)
		argnum = srange
		argname = smcname + "_arg_" + argnum 
		fieldnameargdef = smcname + "_arg_" + argnum + "_field"
		arglst[smcname].append(argnum)
		argnumfield[smcname][argname] = {}
		argfieldname[smcname][argname] = []
		argstartbit[smcname][argname] = {}
		argendbit[smcname][argname] = {}
		argdefval[smcname][argname] = {}
		argnumfield[smcname][argname][fieldnameargdef] = argnum
		argfieldname[smcname][argname].append(fieldnameargdef)
		argstartbit[smcname][argname][fieldnameargdef] = str(0)
		argendbit[smcname][argname][fieldnameargdef] = str(63)
		argdefval[smcname][argname][fieldnameargdef] = sinstr.group(2)
		lcon = 1
		argoccupy[argnum] = 1
		if seq != 1:
			if seq != 2:
				print("Error: out of sequence for arg(value)",end=" ")
				print("arg",end=" ")
				print(argnum,end=" ")
				print("for argname",end=" ")
				print(argname)
				sys.exit()
			else:
				seq = 2
		else:
			seq = seq + 1
	sinstr = re.search(r'^arg(\d+)-arg(\d+)\s*=\s*(0x[a-fA-F0-9]+|\d+)$',sl)
	if sinstr:
		srange = int(sinstr.group(1))
		erange = int(sinstr.group(2))
		argrangename = smcname + "_args_"+ sinstr.group(1) + "_" + sinstr.group(2)
		for i in range((erange - srange) + 1):
			if str(srange + i) in argoccupy:
				print("Error: register already specified for SMC call",end=" ")
				print(smcname,end=" ")
				print("argument",end=" ")
				print(str(srange + i))
				sys.exit()
			argnum = srange + i
			argname = smcname + "_arg_" + str(argnum)
			fieldnameargdef = smcname + "_arg_" + str(argnum) + "_field"
			arglst[smcname].append(argnum)
			argnumfield[smcname][argname] = {}
			argfieldname[smcname][argname] = []
			argstartbit[smcname][argname] = {}
			argendbit[smcname][argname] = {}
			argdefval[smcname][argname] = {}
			argnumfield[smcname][argname][fieldnameargdef] = str(argnum)
			argfieldname[smcname][argname].append(fieldnameargdef)
			argstartbit[smcname][argname][fieldnameargdef] = str(0)
			argendbit[smcname][argname][fieldnameargdef] = str(63)
			argdefval[smcname][argname][fieldnameargdef] = sinstr.group(3)
			argoccupy[str(argnum)] = 1
		lcon = 1
		if seq != 1:
			if seq != 2:
				print("Error: out of sequence for arg(value)",end=" ")
				print("arg",end=" ")
				print(argnum,end=" ")
				print("for argname",end=" ")
				print(argname)
				sys.exit()
			else:
				seq = 2
		else:
			seq = seq + 1
	sinstr = re.search(r'^field:([a-zA-Z0-9_]+):\[(\d+),(\d+)\]\s*=\s*(0x[a-fA-F0-9]+|\d+)$',sl)
	if sinstr:
		for fs in fieldoccupy:
			if(((fs[0] <= int(sinstr.group(3))) and (fs[0] >= int(sinstr.group(2)))) or
			((fs[1] <= int(sinstr.group(3))) and (fs[1] >= int(sinstr.group(2))))):
				print("Error: field overlap",end=" ")
				print(smcname,end=" ")
				print(argname,end=" ")
				print(sinstr.group(1),end=" ")
				print(fs[0],end=" ")
				print(fs[1],end=" ")
				print(" with",end=" ")
				print(sinstr.group(2),end=" ")
				print(sinstr.group(3))
				sys.exit()
		argnumfield[smcname][argname][sinstr.group(1)] = argnum
		argfieldname[smcname][argname].append(sinstr.group(1))
		argstartbit[smcname][argname][sinstr.group(1)] = sinstr.group(2)
		argendbit[smcname][argname][sinstr.group(1)] = sinstr.group(3)
		argdefval[smcname][argname][sinstr.group(1)] = sinstr.group(4)
		flist = []
		flist.append(int(sinstr.group(2)))
		flist.append(int(sinstr.group(3)))
		fieldoccupy.append(flist)
		lcon = 1
		if seq != 2:
			print("Error: out of sequence for field")
			sys.exit()
	if not lcon:
		print("Error: malformed line at",end=" ")
		print(sl)
		sys.exit()
if(seq != 2):
	print("incorrect ending for smc specification")
	sys.exit()

asdfile = open("./include/arg_struct_def.h", "w")
hline = "/*\n"
hline += " * Copyright (c) 2024, Arm Limited. All rights reserved.\n"
hline += " *\n"
hline += " * SPDX-License-Identifier: BSD-3-Clause\n"
hline += " */\n"
hline += "\n"
hline += "#ifndef ARG_STRUCT_DEF_H\n"
hline += "#define ARG_STRUCT_DEF_H\n"
hline += "\n"
asdfile.write(hline)
smccount = 0
argcount = 0
for sn in argfieldname:
	hline = "#define "
	hline += sn
	hline += " "
	hline += str(smccount)
	hline += "\n"
	asdfile.write(hline)
	smccount = smccount + 1
smccount = smccount - 1
hline = "#define MAX_SMC_CALLS "
hline += str(smccount)
hline += "\n"
asdfile.write(hline)
asdfile.write("\n")
for sn in arglst:
	for an in arglst[sn]:
		hline = "#define "
		hline += sn
		hline += "_ARG" + str(an) + " " + str(argcount)
		hline += "\n"
		asdfile.write(hline)
		argcount = argcount + 1
argcount = argcount - 1
hline = "#define MAX_ARG_LENGTH "
hline += str(argcount)
hline += "\n\n"
asdfile.write(hline)
for sn in argnumfield:
	for ag in argnumfield[sn]:
		fieldcount = 0
		for fn in argnumfield[sn][ag]:
			hline = "#define " + sn + "_ARG" + str(argnumfield[sn][ag][fn]) + "_" + fn.upper() + "_CNT " + str(fieldcount)
			hline += "\n"
			asdfile.write(hline)
			fieldcount = fieldcount + 1
fieldcount = 0
hline = "\n\n"
asdfile.write(hline)
for sn in argnumfield:
	for ag in argnumfield[sn]:
		for fn in argnumfield[sn][ag]:
			hline = "#define " + sn + "_ARG" + str(argnumfield[sn][ag][fn]) + "_" + fn.upper() + " " +  str(fieldcount)
			hline += "\n"
			asdfile.write(hline)
			fieldcount = fieldcount + 1
hline = "\n#endif /* ARG_STRUCT_DEF_H */\n"
asdfile.write(hline)

asdfile.close()
faafile  = open("./include/field_specification.h", "w")
hline = "struct fuzzer_arg_def fuzzer_arg_array[] = {\n"
faafile.write(hline)
ifield = 1
for sn in argfieldname:
	for an in argfieldname[sn]:
		for fn in argfieldname[sn][an]:
			if not ifield:
				hline = ",\n"
				hline += "{ .bitw = "
			else:
				hline = "{ .bitw = "
			hline += str((int(argendbit[sn][an][fn]) - int(argstartbit[sn][an][fn])) + 1)
			hline += ", .bitst = " + argstartbit[sn][an][fn] + ", .bnames = \""
			hline += fn + "\", .defval = " + argdefval[sn][an][fn] + ", .regnum = "
			hline += argnumfield[sn][an][fn] + ", .smcname = \"" + sn + "\", .smcargname = \""
			hline += an + "\" }"
			ifield = 0
			faafile.write(hline)
hline = " };\n\n"
faafile.write(hline)
lc = 0
hline = "struct fuzzer_arg_arange fuzzer_arg_array_lst[] = {" + "\n"
faafile.write(hline)
ifield = 1
for sn in argfieldname:
	for ag in argfieldname[sn]:
		if not ifield:
			hline = ",\n"
			hline += "{ .arg_span = {" + str(lc) + "," + str(len(argfieldname[sn][ag]) - 1 + lc) + "} }"
		else :
			hline = "{ .arg_span = {" + str(lc) + "," + str(len(argfieldname[sn][ag]) - 1 + lc) + "} }"
		ifield = 0
		faafile.write(hline)
		lc = lc + len(argfieldname[sn][ag])
hline = " };\n\n"
faafile.write(hline)
hline = "int fuzzer_arg_array_range[] = {" + "\n"
faafile.write(hline)
lc = 0
ifield = 1
hline = ""
for sn in argfieldname:
	if not ifield:
		hline += ","
		hline += str(len(argfieldname[sn]))
	else:
		hline += str(len(argfieldname[sn]))
	lc = lc + 1
	ifield = 0
	if lc == 20:
		hline += "\n"
		lc = 0
hline += "};\n\n"
faafile.write(hline)
hline = "int fuzzer_arg_array_start[] = {" + "\n"
faafile.write(hline)
lc = 0
ifield = 1
cargs = 0
hline = ""
for sn in argfieldname:
	if not ifield:
		hline += ","
		hline += str(cargs)
	else:
		hline += str(cargs)
	cargs = cargs + len(argfieldname[sn])
	lc = lc + 1
	ifield = 0
	if lc == 20:
		hline += "\n"
		lc = 0
hline += "};\n\n"
faafile.write(hline)
hline = "int fuzzer_fieldarg[] = {" + "\n"
faafile.write(hline)
blist = 1
hline = "\t"
for sn in argnumfield:
	for ag in argnumfield[sn]:
		for fn in argnumfield[sn][ag]:
			if not blist:
				hline += ",\n\t"
			blist = 0
			hline += sn + "_ARG" + str(argnumfield[sn][ag][fn])
			faafile.write(hline)
			hline = ""
hline = "\n};"
faafile.write(hline)
hline = "\n\n"
faafile.write(hline)
hline = "int fuzzer_fieldcall[] = {" + "\n"
faafile.write(hline)
blist = 1
hline = "\t"
for sn in argnumfield:
	for ag in argnumfield[sn]:
		for fn in argnumfield[sn][ag]:
			if not blist:
				hline += ",\n\t"
			blist = 0
			hline += sn
			faafile.write(hline)
			hline = ""
hline = "\n};"
faafile.write(hline)
hline = "\n\n"
faafile.write(hline)
hline = "int fuzzer_fieldfld[] = {" + "\n"
faafile.write(hline)
blist = 1
hline = "\t"
for sn in argnumfield:
	for ag in argnumfield[sn]:
		for fn in argnumfield[sn][ag]:
			if not blist:
				hline += ",\n\t"
			blist = 0
			hline += sn + "_ARG" + str(argnumfield[sn][ag][fn]) + "_" + fn.upper() + "_CNT"
			faafile.write(hline)
			hline = ""
hline = "\n};"

faafile.write(hline)

faafile.close()
