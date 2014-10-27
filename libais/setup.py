from distutils.core import setup, Extension

SOURCES = ['libais-python.c', 'libais.c', 'gpsd_json.c', 'driver_ais.c', 'bits.c']

libais = Extension('libais', sources = SOURCES)

setup (name = 'libais',
       version = '1.0',
       description = 'Package for decoding of AIS/NMEA sentences.',
       ext_modules = [libais])
