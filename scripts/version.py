# Contains the openMSX version number and versioning related functions.

from executils import captureStdout
from makeutils import filterLines

from os import makedirs
from os.path import isdir

import re

# Name used for packaging.
packageName = 'openmsx-debugger'

# Version number.
packageVersionNumber = '0.10.0'
# Note: suffix should be empty or with dash, like "-rc1" or "-test1"
packageVersionSuffix = ''
packageVersion = packageVersionNumber + packageVersionSuffix

# Is this a release version ("True") or development version ("False").
releaseFlag = False

def _extractRevisionFromStdout(log, command, regex):
	text = captureStdout(log, command)
	if text is None:
		# Error logging already done by captureStdout().
		return None
	# pylint 0.18.0 somehow thinks captureStdout() returns a list, not a string.
	lines = text.split('\n') # pylint: disable-msg=E1103
	for revision, in filterLines(lines, regex):
		print('Revision number found by "%s": %s' % (command, revision), file = log)
		return revision
	else:
		print('Revision number not found in "%s" output:' % command, file = log)
		print(text, file = log)
		return None

def extractGitRevision(log):
	return _extractRevisionFromStdout(
		log, 'git describe --always --dirty', r'\S+?-(\S+)$'
		)

def extractNumberFromGitRevision(revisionStr):
	if revisionStr is None:
		return None
	return re.match(r'(\d+)+', revisionStr).group(0)

_cachedRevision = False # because None is a valid result

def extractRevision():
	global _cachedRevision
	if _cachedRevision is not False:
		return _cachedRevision
	if releaseFlag:
		# Not necessary, we do not append revision for a release build.
		return None
	if not isdir('derived'):
		makedirs('derived')
	with open('derived/version.log', 'w', encoding='utf-8') as log:
		print('Extracting version info...', file=log)
		print('Package version: %s' % packageVersion, file=log)
		print('Extracting revision info...', file=log)
		revision = extractGitRevision(log)
		print('Revision string: %s' % revision, file=log)
		revisionNumber = extractNumberFromGitRevision(revision)
		print('Revision number: %s' % revisionNumber, file=log)
	_cachedRevision = revision
	return revision

def extractRevisionNumber():
	return int(extractNumberFromGitRevision(extractRevision()) or 1)

def extractRevisionString():
	return extractRevision() or 'unknown'

def getDetailedVersion():
	if releaseFlag:
		return packageVersion
	else:
		return '%s-%s' % (packageVersion, extractRevisionString())

def getVersionedPackageName():
	if releaseFlag:
		return '%s-%s' % (packageName, packageVersion)
	else:
		return '%s-%s-%s' % (
			packageName, packageVersion, extractRevisionString()
			)
