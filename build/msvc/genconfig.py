# Generates configuration headers for VC++ builds

import sys
import os.path
import outpututils
import win_resource
import version2code

#
# platform: one of { Win32, x64 }
# configuration: one of { Debug, Developer, Release }
# outputPath: the location in which to generate config files
#
def genConfig(platform, configuration, outputPath):
	
	#
	# resource-info.hh
	#
	resourceInfoHeader = os.path.join(outputPath, 'resource-info.h')
	generator = win_resource.iterResourceHeader()
	outpututils.rewriteIfChanged(resourceInfoHeader, generator)

	#
	# version.ii
	#
	versionHeader = os.path.join(outputPath, 'version.ii')
	generator = version2code.iterVersionInclude()
	outpututils.rewriteIfChanged(versionHeader, generator)

if len(sys.argv) == 4:
	genConfig(sys.argv[1], sys.argv[2], sys.argv[3])
else:
	print >> sys.stderr, 'Usage: python genconfig.py platform configuration outputPath'
	sys.exit(2)
