import os 
dir_path = os.path.dirname(os.path.realpath(__file__))

def FlagsForFile( filename, **kwargs ):
	return {
		'flags': [ '-x', 'c++', '-std=gnu++1z', '-Wall',  '-Werror', '-I', dir_path],
	}
