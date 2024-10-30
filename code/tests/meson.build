if get_option('with_test').enabled()
    run_command(['python3', 'tools' / 'generate-runner.py'], check: true)

    c_test_src = ['unit_runner.c']
    cpp_test_src = ['unit_runner.cpp']

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

    test('C API test cases',   c_pizza)
    test('C++ API test cases', cpp_pizza)
endif
