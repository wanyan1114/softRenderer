add_executable(sr_viewer
    main.cpp)

target_compile_features(sr_viewer PRIVATE cxx_std_20)

target_link_libraries(sr_viewer
    PRIVATE
        sr_engine)
