from distutils.core import setup
from distutils.extension import Extension

ext_files = ["pyreBloom/pyreBloom.pyx"]

kwargs = {}

try:
    from Cython.Distutils import build_ext
    print "Building from Cython"
    kwargs['cmdclass'] = {'build_ext': build_ext}
except ImportError:
    ext_files.append("pyreBloom/pyreBloom.c")
    print "Building from C"

ext_modules = [Extension("pyreBloom", ext_files)]

setup(
    name = 'pyreBloom',
    version = "0.1.0",
    author = "Dan Lecocq",
    author_email = "dan@seomoz.org",
    license = "MIT License",
    ext_modules = ext_modules,
    classifiers = [
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: C',
        'Programming Language :: Cython',
        'Programming Language :: Python',
        'Topic :: Software Development :: Libraries :: Python Modules',
        ],
  **kwargs
)
 
