c_test_cubes = [
    'crabdb', 'crabql', 'crabsearch', 'crabsync'
]

foreach cube : c_test_cubes
    c_test_src += ['test_' + cube + '.c']
endforeach

c_pizza = executable('runner-c', c_test_src,
    include_directories: dir,
    dependencies: [
        dependency('fossil-test'),
        dependency('fossil-mock'),
        fossil_crabdb_dep])
