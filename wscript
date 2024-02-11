from waflib.Build import BuildContext, CleanContext, InstallContext, UninstallContext
from waflib.Context import Context
import subprocess, os

def options(opt):
	opt.load('compiler_c')

def configure(conf):
	def mod_flags(base, sub, add):
		res = base.copy()
		for flag in sub:
			try:
				idx = res.index(flag)
			except ValueError:
				continue
			del res[idx]
		res.extend(add)
		return res
	base_cflags = [
		'-Wall',
		'-Wextra',
		'-Wformat=2',
		'-Werror',
		'-Wno-unused-parameter',
		'-std=c2x',
		'-fno-strict-aliasing',
		'-pthread'
	]
	base_ldflags = [
		'-Llib',
		'-fno-strict-aliasing',
		'-pthread'
	]
	if 'GITHUB_RUN_ID' in os.environ:
		print('We seem to be running on github actions, applying compat kludges!')
		CLANG = 'clang-15'
		GCC = 'gcc-13'
		XGCC = None
		XAR = None
	else:
		CLANG = 'clang'
		GCC = 'gcc'
		XGCC = 'x86_64-w64-mingw32-gcc'
		XAR = 'x86_64-w64-mingw32-ar'
	conf.setenv('coverage')
	conf.env.CC = GCC
	conf.env.LD = GCC
	conf.load('compiler_c')
	conf.env.CFLAGS = mod_flags(base_cflags, [], [
		'-ggdb3',
		'-pg',
		'-fno-eliminate-unused-debug-types',
		'--coverage',
		'-fsanitize=address,object-size,vla-bound',
		'-O1',
		'-DDEBUG'
	])
	conf.env.LDFLAGS = mod_flags(base_ldflags, [], [
		'-ggdb3',
		'-pg',
		'-fno-eliminate-unused-debug-types',
		'--coverage',
		'-fsanitize=address,object-size,vla-bound'
	])

	conf.setenv('debug')
	conf.env.CC = CLANG
	conf.env.LD = CLANG
	conf.load('compiler_c')
	conf.env.CFLAGS = mod_flags(base_cflags, [], [
		'-ggdb3',
		'-fno-eliminate-unused-debug-types',
		'-fno-omit-frame-pointer',
		'-fno-optimize-sibling-calls',
		'-fsanitize=memory,bool,builtin,bounds,enum,function,integer,nonnull-attribute,nullability,pointer-overflow,returns-nonnull-attribute,shift,unsigned-shift-base,unreachable,vla-bound',
		'-DDEBUG'
	])
	conf.env.LDFLAGS = mod_flags(base_ldflags, [], [
		'-ggdb3',
		'-fno-eliminate-unused-debug-types',
		'-fno-omit-frame-pointer',
		'-fno-optimize-sibling-calls',
		'-fsanitize=memory,bool,builtin,bounds,enum,function,integer,nonnull-attribute,nullability,pointer-overflow,returns-nonnull-attribute,shift,unsigned-shift-base,unreachable,vla-bound',
		'-O1'
	])

	conf.setenv('release')
	conf.env.CC = GCC
	conf.env.LD = GCC
	conf.load('compiler_c')
	conf.env.CFLAGS = mod_flags(base_cflags, [], [
		'-O3',
		'-DNDEBUG'
	])
	conf.env.LDFLAGS = mod_flags(base_ldflags, [], [
		'-O3',
		'-flto'
	])

	conf.setenv('valgrind')
	conf.env.CC = GCC
	conf.env.LD = GCC
	conf.load('compiler_c')
	conf.env.CFLAGS = mod_flags(base_cflags, [], [
		'-g',
		'-fno-eliminate-unused-debug-types',
		'-O1',
		'-DDEBUG'
	])
	conf.env.LDFLAGS = mod_flags(base_ldflags, [], [
		'-g',
		'-fno-eliminate-unused-debug-types',
		'-O1'
	])

	if XGCC is not None:
		conf.setenv('windows')
		conf.env.CC = XGCC
		conf.env.LD = XGCC
		conf.env.AR = XAR
		conf.load('compiler_c')
		conf.env.CFLAGS = mod_flags(base_cflags, ['-pthread'], [
			'-lwinpthread',
			'-O3',
			'-DNDEBUG'
		])
		conf.env.LDFLAGS = mod_flags(base_ldflags, ['-pthread'], [
			'-flto',
			'-lwinpthread',
			'-static-libgcc',
			'-static-libstdc++'
		])
	else:
		print("Skipping variant 'windows' on github actions")

def build(ctx):
	if not ctx.variant:
		ctx.fatal('must specify a variant like "waf build_debug" or "waf build_release"; try "waf --help"')
	simple_stlib_sources = ctx.path.ant_glob('src/lib/*.c')
	compound_stlib_dirs = ctx.path.ant_glob('src/lib/*', dir=True, src=False)
	simple_test_sources = ctx.path.ant_glob('src/test/*.c')
	compound_test_dirs = ctx.path.ant_glob('src/test/*', dir=True, src=False)
	simple_binary_sources = ctx.path.ant_glob('src/bin/*.c')
	compound_binary_dirs = ctx.path.ant_glob('src/bin/*', dir=True, src=False)
	stlib_src_dir = ctx.path.find_dir('src/lib')
	test_src_dir = ctx.path.find_dir('src/test')
	binary_src_dir = ctx.path.find_dir('src/bin')

	stlibs = []

	for source_file in simple_stlib_sources:
		stlib_name = source_file.path_from(stlib_src_dir)[:-2] # strip .c
		ctx.stlib(
			source = 'src/lib/' + source_file.name,
			target = stlib_name,
			includes = ['include'],
			lib = ['m', 'SDL2'],
			install_path = '${PREFIX}/lib'
		)
		stlibs.append(stlib_name)
	
	for compound in compound_stlib_dirs:
		stlib_name = compound.path_from(stlib_src_dir)
		compound_sources = compound.ant_glob('*.c')
		prefix = 'src/lib/' + compound.name + '/'
		ctx.stlib(
			source = [prefix + n.name for n in compound_sources],
			target = stlib_name,
			includes = ['include'],
			lib = ['m', 'SDL2'],
			install_path = '${PREFIX}/lib'
		)
		stlibs.append(stlib_name)
	
	for source_file in simple_test_sources:
		test_name = source_file.path_from(test_src_dir)[:-2] # strip .c
		ctx.program(
			source = 'src/test/' + source_file.name,
			target = test_name,
			includes = ['include'],
			lib = ['m', 'SDL2'],
			install_path = None,
			use = stlibs
		)
	
	for compound in compound_test_dirs:
		test_name = compound.path_from(test_src_dir)
		compound_sources = compound.ant_glob('*.c')
		prefix = 'src/test/' + compound.name + '/'
		ctx.program(
			source = [prefix + n.name for n in compound_sources],
			target = test_name,
			includes = ['include'],
			lib = ['m', 'SDL2'],
			install_path = None,
			use = stlibs
		)
	
	for source_file in simple_binary_sources:
		binary_name = source_file.path_from(test_src_dir)[:-2] # strip .c
		ctx.program(
			source = 'src/bin/' + source_file.name,
			target = binary_name,
			includes = ['include'],
			lib = ['m', 'SDL2'],
			use = stlibs
		)
	
	for compound in compound_binary_dirs:
		binary_name = source_file.path_from(test_src_dir)
		compound_sources = compound.ant_glob('*.c')
		prefix = 'src/bin/' + compound.name + '/'
		ctx.program(
			source = [prefix + n.name for n in compound_sources],
			target = binary_name,
			includes = ['include'],
			lib = ['m', 'SDL2'],
			use = stlibs
		)
	
	header_dir = ctx.path.find_dir('include')
	ctx.install_files('${PREFIX}/include', header_dir.ant_glob('**/*.h'), relative_trick=True)

def test(ctx):
	if not ctx.variant:
		ctx.fatal('must specify a variant like "waf test_debug" or "waf test_release"; try "waf --help"')
	cmd = ['test/test.py', '-v', ctx.variant]
	if ctx.variant == 'valgrind':
		cmd = ['test/test.py', '-g', '-v', 'valgrind']
	child = subprocess.Popen(cmd)
	child.wait()
	if child.returncode != 0:
		ctx.fatal('tests failed')
	if ctx.variant == 'coverage':
		ctx.exec_command('geninfo build/coverage/src/lib/crater')
		ctx.exec_command('find build/coverage/src/lib/crater -name "*.info" | xargs genhtml --num-spaces 4 --css-file resources/lcov.css --html-prolog resources/lcov_prolog.html -o cov | grep -oP "(?<=lines\\.{6,6}: )\\d+\\.\d+" | xargs ./cov_shield.py > coverage.svg')

def docs(ctx):
	ctx.exec_command('doxygen doxygen.conf')

class TestContext(Context):
	cmd = 'test'
	fun = 'test'

for x in 'coverage debug release valgrind windows'.split():
	for y in (BuildContext, CleanContext, InstallContext, UninstallContext, TestContext):
		name = y.__name__.replace('Context','').lower()
		class tmp(y):
			cmd = name + '_' + x
			variant = x

