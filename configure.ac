AC_INIT([configure.ac])
 
m4_include([OpenSBE.ac])

OSBE_ENABLE_FEATURE([CmdDep])
OSBE_ENABLE_FEATURE([CXX/CXX])
OSBE_ENABLE_FEATURE([CXX/Ar])
OSBE_ENABLE_FEATURE([CXX/So])
OSBE_ENABLE_FEATURE([CXX/Ex])
OSBE_ENABLE_FEATURE([CXX/Inl])

OSBE_ENABLE_FEATURE([CXX/XML/XSD])
OSBE_ENABLE_FEATURE([CXX/Corba])
OSBE_ENABLE_FEATURE([CXX/DR/ProtoBuf])
OSBE_ENABLE_FEATURE([CXX/Def])
OSBE_ENABLE_FEATURE([CXX/AutoDep])
OSBE_ENABLE_FEATURE([Documentation/Doxygen])

osbe_ask_for_ne_string "Please enter unixcommons root path" \
                       "unixcommons root path" \
                       unixcommons_root \
                       "unixcommons_root"
osbe_ask_for_ne_string "Please enter unixcommons include path(s)" \
                       "unixcommons include path(s)" \
                       unixcommons_include \
                       "unixcommons_include"
osbe_ask_for_ne_string "Please enter unixcommons CORBA include path(s)" \
                       "unixcommons CORBA include path(s)" \
                       unixcommons_corba_include \
                       "unixcommons_corba_include"
osbe_ask_for_ne_string "Please enter unixcommons library path(s)" \
                       "unixcommons library path(s)" \
                       unixcommons_lib \
                       "unixcommons_lib"

unixcommons_def_initialize()
{
  root="$unixcommons_root"
  if test "X$so_files" = "XCORBACommons"; then
    include="$unixcommons_include $unixcommons_corba_include"
  else
    include="$unixcommons_include"
  fi
  lib="$unixcommons_lib"
}
unixcommons_def_initializer=unixcommons_def_initialize


use_occi="use_occi"
osbe_ask_for_ne_string "Please enter \"use_occi\" if needed, or \"occi_not_used\" otherwise" \
                       "occi" \
                       use_occi \
                       "occi_status"
if [[ "$use_occi" == "use_occi" ]] ; then
  CPPFLAGS="$CPPFLAGS -D_USE_OCCI"
  build_tests="build_tests"
  osbe_ask_for_ne_string "Please enter \"build_tests\" if needed, or \"dont_build_tests\" otherwise" \
                         "tests" \
                         build_tests \
                         "build_tests_status"
else
  build_tests="dont_build_tests"
fi


unixcommons_lib_path="$unixcommons_root/$unixcommons_lib"

AC_SUBST(use_occi)
AC_SUBST(build_tests)
AC_SUBST(unixcommons_lib_path)

osbe_ask_for_ne_string "Please enter unixcommons idls directories" \
                       "unixcommons idls directories" \
                       unixcommons_idls_path \
                       "unixcommons_idls_path"
AC_SUBST(unixcommons_idls_path)

AC_CONFIG_FILES([Plain/Plain.post.rules])

m4_include([dir.ac])

OSBE_OUTPUT
