cpp_test_cubes = [
    'crabdb', 'crabql', 'crabsearch', 'crabsync'
]

foreach cube : cpp_test_cubes
    cpp_test_src += ['test_' + cube + '.cpp']
endforeach

cpp_pizza = executable('runner-cpp', cpp_test_src,
    include_directories: dir,
    dependencies: [
        dependency('fossil-test'),
        dependency('fossil-mock'),
        fossil_crabdb_dep])
