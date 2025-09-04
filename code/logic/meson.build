dir = include_directories('.')

fossil_crabdb_lib = library('fossil_crabdb',
    files(
        'myshell.c',
        'noshell.c',
        'fileshell.c',
        'timeshell.c',
        'cacheshell.c',
        'storage.c',
        'bluecrab.c'
        ),
    install: true,
    include_directories: dir
)

fossil_crabdb_dep = declare_dependency(
    link_with: [fossil_crabdb_lib],
    include_directories: dir
)

meson.override_dependency('fossil-crabdb', fossil_crabdb_dep)