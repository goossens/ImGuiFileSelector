#	FileSelector - A file select dialog for Dear ImGui.
#	Copyright (c) 2026 Johan A. Goossens. All rights reserved.
#
#	This work is licensed under the terms of the MIT license.
#	For a copy, see <https://opensource.org/licenses/MIT>.

import json, os

# switch directory to script location
scriptPath = os.path.abspath(__file__)
os.chdir(os.path.dirname(scriptPath))

# read glyph data
with open("icons.json", "r") as file:
	jsonData = json.load(file)

glyphs = []

for name, code in jsonData.items():
	cppName = ""
	upper = False

	for char in name:
		if char == '-':
			upper = True

		elif upper:
			cppName += char.upper()
			upper = False

		else:
			cppName += char

	unicode = f"u8\"\\u{code:04x}\""
	glyphs.append(f"\tstatic constexpr Codepoint {cppName} = {unicode};")

# convert font to C++ data
with open("icons.ttf", "rb") as input:
	contents = bytearray(input.read())

bytesPerLine = 16
size = len(contents)
data = f"uint8_t FileSelectorIcons::iconData[{size}] = {{\n\t"

for i, b in enumerate(contents):
	data += "0x" + format(b, "02x")

	if i != size - 1:
		data += ","

		if (i + 1) % bytesPerLine == 0:
			data += "\n\t"

		else:
			data += " "

data += "\n};\n"

# read templates
with open("template.h", "r") as file:
	header = file.read()

with open("template.cpp", "r") as file:
	source = file.read()

# populate templates
header = header.replace("GLYPHS", "\n".join(glyphs))
header = header.replace("SIZE", str(size))
source = source.replace("DATA", data)

## write results
with open("../../FileSelectorIcons.h", "w") as output:
	output.write(header)

with open("../../FileSelectorIcons.cpp", "w") as output:
	output.write(source)
