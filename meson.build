project(
  'droidmedia', 'c',
  default_options : ['warning_level=1', 'buildtype=debugoptimized'],
)

droidmedia_version = meson.project_version()
droidmedia = meson.project_name()

cc = meson.get_compiler('c')

root_dir = include_directories('.')

droidmedia_source = [
  'hybris.c',
]

libdroidmedia_a = static_library('droidmedia',
                                 droidmedia_source,
                                 install: true,
                                 dependencies: cc.find_library('dl', required : true))

droidmedia_headers = [
    'droidmediacamera.h',
    'droidmediacodec.h',
    'droidmediaconstants.h',
    'droidmediaconvert.h',
    'droidmedia.h',
    'droidmediarecorder.h'
]

install_headers(droidmedia_headers, subdir : meson.project_name())
install_data('hybris.c', install_dir : get_option('datadir') / meson.project_name())

pkg = import('pkgconfig')
pkg.generate(libdroidmedia_a,
             libraries: ['-ldl'],
             subdirs: ['droidmedia'])
