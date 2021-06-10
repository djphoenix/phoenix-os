add_link_options("-nostdlib")
add_compile_options("-nostdlib")

add_link_options("-g")
add_compile_options("-g")

add_link_options("--hash-style=sysv")

add_compile_options("-m64")

include(CheckCXXCompilerFlag)
macro (CHECK_ADD_CXX_COMPILER_FLAG _FLAG _VAR)
  check_cxx_compiler_flag("${_FLAG}" ${_VAR})
  if (${_VAR})
    add_compile_options(${_FLAG})
  endif()
endmacro ()

check_add_cxx_compiler_flag("-pipe" HAVE_CXX_PIPE)

check_add_cxx_compiler_flag("-fpic" HAVE_FPIC)
check_add_cxx_compiler_flag("-fpie" HAVE_FPIE)
check_add_cxx_compiler_flag("-ffreestanding" HAVE_FFREESTANDING)
check_add_cxx_compiler_flag("-fno-exceptions" HAVE_FNO_EXCEPTIONS)
check_add_cxx_compiler_flag("-fno-rtti" HAVE_FNO_RTTI)
check_add_cxx_compiler_flag("-ffunction-sections" HAVE_FFUNCTION_SECTIONS)
check_add_cxx_compiler_flag("-fdata-sections" HAVE_FDATA_SECTIONS)
check_add_cxx_compiler_flag("-fshort-wchar" HAVE_FSHORT_WCHAR)

check_add_cxx_compiler_flag("-fanalyzer" HAVE_FANALYZER)
check_add_cxx_compiler_flag("-Wanalyzer-too-complex" HAVE_WANALYZER_TOO_COMPLEX)
check_add_cxx_compiler_flag("-Wno-error=analyzer-too-complex" HAVE_WNOERR_ANALYZER_TOO_COMPLEX)

check_add_cxx_compiler_flag("-Wall" HAVE_WALL)
check_add_cxx_compiler_flag("-Wextra" HAVE_WEXTRA)
check_add_cxx_compiler_flag("-Werror" HAVE_WERROR)
check_add_cxx_compiler_flag("-Wpedantic" HAVE_WPEDANTIC)
check_add_cxx_compiler_flag("-Wconversion" HAVE_WCONVERSION)
check_add_cxx_compiler_flag("-Wno-unused-parameter" HAVE_WNO_UNUSED_PARAMETER)
check_add_cxx_compiler_flag("-Wno-unused-command-line-argument" HAVE_WNO_UNUSED_COMMAND_LINE_ARGUMENT)

check_add_cxx_compiler_flag("-Wno-gnu-anonymous-struct" HAVE_WNO_GNU_ANONYMOUS_STRUCT)
check_add_cxx_compiler_flag("-Wno-gnu-empty-struct" HAVE_WNO_GNU_EMPTY_STRUCT)
check_add_cxx_compiler_flag("-Wno-nested-anon-types" HAVE_WNO_NESTED_ANON_TYPES)
check_add_cxx_compiler_flag("-Wno-c99-extensions" HAVE_WNO_C99_EXTENSIONS)

check_add_cxx_compiler_flag("-Wdouble-promotion" HAVE_WDOUBLE_PROMOTION)
check_add_cxx_compiler_flag("-Wfloat-equal" HAVE_WFLOAT_EQUAL)

check_add_cxx_compiler_flag("-Wformat=2" HAVE_WFORMAT2)
check_add_cxx_compiler_flag("-Wformat-overflow=2" HAVE_WFORMAT_OVERFLOW2)
check_add_cxx_compiler_flag("-Wformat-nonliteral" HAVE_WFORMAT_NONLITERAL)
check_add_cxx_compiler_flag("-Wformat-security" HAVE_WFORMAT_SECURITY)
check_add_cxx_compiler_flag("-Wformat-signedness" HAVE_WFORMAT_SIGNEDNESS)
check_add_cxx_compiler_flag("-Wformat-truncation=2" HAVE_WFORMAT_TRUNCATION2)

check_add_cxx_compiler_flag("-Wnull-dereference" HAVE_WNULL_DEREFERENCE)
check_add_cxx_compiler_flag("-Wuninitialized" HAVE_WUNITIALIZED)
check_add_cxx_compiler_flag("-Wimplicit-fallthrough=2" HAVE_WIMPLICIT_FALLTHROUGH)

check_add_cxx_compiler_flag("-Wstrict-aliasing" HAVE_WSTRICT_ALIASING)
check_add_cxx_compiler_flag("-Wstrict-overflow" HAVE_WSTRICT_OVERFLOW)
check_add_cxx_compiler_flag("-Wstringop-overflow=3" HAVE_WSTRINGOP_OVERFLOW3)

check_add_cxx_compiler_flag("-Wsuggest-attribute=pure" HAVE_WSUGGEST_ATTRIBUTE_PURE)
check_add_cxx_compiler_flag("-Wsuggest-attribute=cold" HAVE_WSUGGEST_ATTRIBUTE_COLD)
check_add_cxx_compiler_flag("-Wsuggest-attribute=const" HAVE_WSUGGEST_ATTRIBUTE_CONST)
check_add_cxx_compiler_flag("-Wsuggest-attribute=noreturn" HAVE_WSUGGEST_ATTRIBUTE_NORETURN)
check_add_cxx_compiler_flag("-Wsuggest-attribute=malloc" HAVE_WSUGGEST_ATTRIBUTE_MALLOC)
check_add_cxx_compiler_flag("-Wsuggest-attribute=format" HAVE_WSUGGEST_ATTRIBUTE_FORMAT)
check_add_cxx_compiler_flag("-Wmissing-noreturn" HAVE_WMISSING_NORETURN)
check_add_cxx_compiler_flag("-Wmissing-format-attribute" HAVE_WMISSING_FORMAT_ATTRIBUTE)
check_add_cxx_compiler_flag("-Wno-error=suggest-attribute=pure" HAVE_WNOERR_SUGGEST_ATTRIBUTE_PURE)
check_add_cxx_compiler_flag("-Wno-error=suggest-attribute=format" HAVE_WNOERR_SUGGEST_ATTRIBUTE_FORMAT)
check_add_cxx_compiler_flag("-Wno-error=missing-format-attribute" HAVE_WNOERR_MISSING_FORMAT_ATTRIBUTE)
check_add_cxx_compiler_flag("-Wno-error=unused-private-field" HAVE_WNOERR_UNUSED_PRIVATE_FIELD)

check_add_cxx_compiler_flag("-Walloca" HAVE_WALLOCA)
check_add_cxx_compiler_flag("-Wno-error=alloca" HAVE_WNOERR_ALLOCA)

check_add_cxx_compiler_flag("-Wunsafe-loop-optimizations" HAVE_WUNSAFE_LOOP_OPTIMIZATIONS)
check_add_cxx_compiler_flag("-Wstack-usage=2048" HAVE_WSTACK_USAGE)
check_add_cxx_compiler_flag("-Wcast-qual" HAVE_WCAST_QUAL)
check_add_cxx_compiler_flag("-Wcast-align=strict" HAVE_WCAST_ALIGN_STRICT)
check_add_cxx_compiler_flag("-Wcast-function-type" HAVE_WCAST_FUNCTION_TYPE)
check_add_cxx_compiler_flag("-Wwrite-strings" HAVE_WWRITE_STRINGS)
check_add_cxx_compiler_flag("-Wredundant-decls" HAVE_WREDUNDANT_DECLS)

check_add_cxx_compiler_flag("-fstack-protector-strong" HAVE_FSTACK_PROTECTOR)
check_add_cxx_compiler_flag("-Wstack-protector" HAVE_WSTACK_PROTECTOR)

check_add_cxx_compiler_flag("-Wproperty-attribute-mismatch" HAVE_WPROPERTY_ATTRIBUTE_MISMATCH)
check_add_cxx_compiler_flag("-Wold-style-cast" HAVE_WOLD_STYLE_CAST)
check_add_cxx_compiler_flag("-Wcomma" HAVE_WCOMMA)
check_add_cxx_compiler_flag("-Wconditional-uninitialized" HAVE_WCONDITIONAL_UNITIALIZED)
check_add_cxx_compiler_flag("-Wzero-as-null-pointer-constant" HAVE_WZERO_AS_NULL_POINTER_CONSTANT)

add_link_options("-O3" "--lto-O3")
add_compile_options("-O3" "-Os" "-flto")
