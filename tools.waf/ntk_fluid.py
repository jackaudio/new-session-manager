from waflib import Task
from waflib.TaskGen import extension

class ntk_fluid(Task.Task):
	color   = 'BLUE'
	ext_out = ['.h']
	run_str = '${NTK_FLUID} -c -o ${TGT[0].abspath()} -h ${TGT[1].abspath()} ${SRC}'

@extension('.fl')
def fluid(self, node):
	"""add the .fl to the source list; the cxx file generated will be compiled when possible"""
	cpp = node.change_ext('.C')
	hpp = node.change_ext('.H')
	self.create_task('ntk_fluid', node, [cpp, hpp])

	if 'cxx' in self.features:
		self.source.append(cpp)

