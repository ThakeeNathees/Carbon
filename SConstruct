#!python
import os, subprocess, sys

opts = Variables([], ARGUMENTS)

# Gets the standard flags CC, CCX, etc.
cbenv = DefaultEnvironment()

# Define our options
opts.Add(EnumVariable('target', "Compilation target", 'debug', ['debug', 'release']))
opts.Add(EnumVariable('platform', "Compilation platform", '', ['', 'windows', 'x11', 'linux', 'osx']))
opts.Add(BoolVariable('use_llvm', "Use the LLVM / Clang compiler", False))
opts.Add(BoolVariable('vsproj', "make a visual studio project", False))
opts.Add(PathVariable('target_path', 'The path to the output library.', 'bin/', PathVariable.PathAccept))
opts.Add(PathVariable('target_name', 'The library/application name.', 'carbon', PathVariable.PathAccept))
opts.Add(EnumVariable('bits', 'output program bits', '64', ['32', '64']))

# Updates the environment with the option variables.
opts.Update(cbenv)

# Process some arguments
if cbenv['use_llvm']:
    cbenv['CC'] = 'clang'
    cbenv['CXX'] = 'clang++'

# debug macro for all platforms
if cbenv['target'] == 'debug':
    cbenv.Append(CPPDEFINES=['DEBUG_BUILD'])
else:
    cbenv.Append(CPPDEFINES=['RELEASE_BUILD'])

# find platform
if cbenv['platform'] == 'linux':
    cbenv['platform'] = 'x11'
if cbenv['platform'] == '':
    if sys.platform == 'win32':
        cbenv['platform'] = 'windows'
    elif sys.platform in ('linux', 'linux2'):
        cbenv['platform'] = 'x11'
    elif sys.platform == 'darwin':
        cbenv['platform'] = 'osx'
    else:
        print("platform(%s) not supported." % sys.platform)
        quit()

# add include dir from outer libs
def add_include_dir(_path):
    dir  = Dir('.').abspath
    path = os.path.join(dir, _path)
    cbenv.Append(CPPPATH=[path])
cbenv.add_include_dir = add_include_dir

cbenv.ide_sources = [] # sources for ide (vsproj)
# add source for vsproj
def add_ide_sources(src):
    if type(src) == str:
        _path = os.path.join(Dir('.').abspath, src)
        cbenv.ide_sources.append(_path)
    elif str(src).endswith('.cpp') or str(src).endswith('.c'):
        _path = os.path.join(Dir('.').abspath, str(src))
        cbenv.ide_sources.append(_path)
    else: ## Glob
        try:
            for _src in src:
                _path = os.path.join(Dir('.').abspath, str(_src))
                cbenv.ide_sources.append(_path)
        except Exception as err:
            print('Error: ', err, '(at add_ide_sources())')
cbenv.add_ide_sources = add_ide_sources

def add_lib(name, path=None):
    if path:
        cbenv.Append(LIBPATH=[path])
    cbenv.Append(LIBS=[name])
cbenv.add_lib = add_lib

# output name suffix, dir
def get_suffix(platform, target, bits):
    return '.%s.%s.%s' % (platform, target, bits)

cbenv["out_suffix"]  = get_suffix(
    cbenv['platform'], cbenv['target'], cbenv['bits'])

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# Check our platform specifics
if cbenv['platform'] == "osx":
    cbenv.Append(CXXFLAGS=['-std=c++17'])
    if cbenv['target'] == 'debug':
        cbenv.Append(CCFLAGS=['-g', '-O2', '-arch', 'x86_64'])
        cbenv.Append(LINKFLAGS=['-arch', 'x86_64'])
    else:
        cbenv.Append(CCFLAGS=['-g', '-O3', '-arch', 'x86_64'])
        cbenv.Append(LINKFLAGS=['-arch', 'x86_64'])

elif cbenv['platform'] == 'x11':
    cbenv.Append(LIBS=['GL', 'GLU', 'dl', 'X11', 'pthread']) 
    cbenv.Append(CXXFLAGS=['-std=c++17'])
    if cbenv['target'] == 'debug':
        cbenv.Append(CCFLAGS=['-fPIC', '-g3', '-Og'])
    else:
        cbenv.Append(CCFLAGS=['-fPIC', '-g', '-O3'])

elif cbenv['platform'] == "windows":
    cbenv.Append(CXXFLAGS=['/std:c++17'])
    # This makes sure to keep the session environment variables on windows,
    # that way you can run scons in a vs 2017 prompt and it will find all the required tools
    cbenv.Append(ENV=os.environ)

    cbenv.Append(CPPDEFINES=['WIN32', '_WIN32', '_WINDOWS', '_CRT_SECURE_NO_WARNINGS'])
    cbenv.Append(CCFLAGS=['-W3', '-GR'])
    cbenv.Append(LINKFLAGS='-SUBSYSTEM:CONSOLE')

    cbenv.Append(LIBS=[
        #'opengl32.lib',
        #'user32.lib',
        #'gdi32.lib',
        #'shell32.lib',
    ])

    if cbenv['target'] == 'debug':
        cbenv.Append(CCFLAGS=['-EHsc', '-MDd', '-ZI'])
        cbenv.Append(LINKFLAGS=['-DEBUG'])
    else:
        cbenv.Append(CCFLAGS=['-O2', '-EHsc', '-MD'])

# includes and libs
#cbenv.includes = []
cbenv.sources  = [       # cpp files
    'main/main_%s.cpp' % cbenv['platform'],
    'main/main.cpp'
]

cbenv.Append(CPPPATH=[Dir('.')]) # include dir
cbenv.Append(LIBPATH=[]) # static lib dir

Export('cbenv')
SConscript('thirdparty/SConstruct')
SConscript('io/SConstruct')
SConscript('core/SConstruct')
SConscript('os/SConstruct')

for src in cbenv.sources:
    add_ide_sources(src)

#for header in cbenv.includes:
#    cbenv.Prepend(CPPPATH=[header])

target = cbenv.Program(target=os.path.join(
    cbenv['target_path'], cbenv['target_name'] + cbenv['out_suffix']),
    source=cbenv.sources,
)

# visual studio targets
def get_vsproj_context():
    vsproj_targets = []
    variants = [] # ["debug|Win32", "debug|x64", "release|Win32", "release|x64"]
    for target in 'debug', 'release':
        for bits in '32', '64':
            vsproj_targets.append(
                os.path.join(
                    cbenv['target_path'],
                    cbenv['target_name'] + cbenv["out_suffix"]
                    + '.exe'
                )
            )
            variants.append(
                target+'|'+('Win32' if bits=='32' else 'x64')
            )
    return vsproj_targets, variants

def msvs_collect_header():
    ret = []
    for _dir in cbenv['CPPPATH']:
        _dir = str(_dir)
        for file in os.listdir(_dir):
            file = os.path.join(_dir, file)
            if os.path.isfile(file) and (file.endswith('.h') or file.endswith('.hpp')):
                ret.append('$(ProjectDir)' + os.path.relpath(file))
            elif os.path.isdir(file):
                ret += _msvc_collect_header_internal_dir(file)
    return ret

def _msvc_collect_header_internal_dir(dir):
    ret = []
    for file in os.listdir(dir):
        file = os.path.join(dir, file)
        if os.path.isfile(file) and (file.endswith('.h') or file.endswith('.hpp')):
            ret.append('$(ProjectDir)' + os.path.relpath(file))
        elif os.path.isdir(file):
            ret += _msvc_collect_header_internal_dir(file)
    return ret

def msvc_build_commandline(commands):
    common_build_prefix = [
        'cmd /V /C set "bits=64"',
        '(if "$(PlatformTarget)"=="x86" (set "bits=32"))',
    ]
    return " ^& ".join(common_build_prefix + [commands])

if cbenv['vsproj']:
    cbenv["MSVSBUILDCOM"] = msvc_build_commandline(
        "scons --directory=\"$(ProjectDir.TrimEnd('\\'))\" platform=windows target=$(Configuration) bits=!bits!"
    )
    cbenv["MSVSREBUILDCOM"] = msvc_build_commandline(
        "scons --directory=\"$(ProjectDir.TrimEnd('\\'))\" platform=windows target=$(Configuration) bits=!bits! vsproj=yes"
    )
    cbenv["MSVSCLEANCOM"] = msvc_build_commandline(
        "scons --directory=\"$(ProjectDir.TrimEnd('\\'))\" --clean platform=windows bits=!bits! target=$(Configuration)"
    )
    targets, variants = get_vsproj_context()
    cbenv.MSVSProject(target = cbenv['target_name'] + cbenv['MSVSPROJECTSUFFIX'],
        srcs = cbenv.ide_sources,
        incs = msvs_collect_header(),
        variant = variants,
        runfile = targets,
        buildtarget = targets,
    )


# Generates help for the -h scons option.
Help(opts.GenerateHelpText(cbenv))
