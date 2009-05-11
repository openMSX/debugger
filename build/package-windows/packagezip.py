import os, sys
import zipfile
import packagewindows

def AddFile(zip, path, zipPath):
	print 'Adding ' + path
	zip.write(path, zipPath, zipfile.ZIP_DEFLATED)

def AddDirectory(zip, root, zipPath):
	for path, dirs, files in os.walk(root):
		if '.svn' in dirs:
			dirs.remove('.svn')  # don't visit .svn directories
		for name in files:
			thisZipPath = zipPath
			if os.path.abspath(root) != os.path.abspath(path):
				thisZipPath = os.path.join(thisZipPath, os.path.relpath(path, root))
			AddFile(zip, os.path.join(path, name), os.path.join(thisZipPath, name))

def PackageZip(info):

	if not os.path.exists(info.packagePath):
		os.mkdir(info.packagePath)

	zipFileName = info.installerFileName + '.zip'
	zipFilePath = os.path.join(info.packagePath, zipFileName)
	if os.path.exists(zipFilePath):
		os.unlink(zipFilePath)

	print 'Generating ' + zipFilePath
	zip = zipfile.ZipFile(zipFilePath, 'w')
	AddFile(zip, os.path.join(info.installPath, 'openmsx-debugger.exe'), 'openmsx-debugger.exe')
	zip.close()
	
	zipFileName = info.installerFileName + '-pdb.zip'
	zipFilePath = os.path.join(info.packagePath, zipFileName)
	if os.path.exists(zipFilePath):
		os.unlink(zipFilePath)

	print 'Generating ' + zipFilePath
	zip = zipfile.ZipFile(zipFilePath, 'w')
	AddFile(zip, os.path.join(info.installPath, 'openmsx-debugger.pdb'), 'openmsx-debugger.pdb')
	zip.close()

if __name__ == '__main__':
	if len(sys.argv) != 4:
		print >> sys.stderr, 'Usage: python packagezip.py platform configuration version'
		# E.g. build\package-windows\package.cmd x64 Release 0.7.1
		sys.exit(2)
	else:
		info = packagewindows.PackageInfo(sys.argv[1], sys.argv[2], sys.argv[3], '')
		PackageZip(info)
