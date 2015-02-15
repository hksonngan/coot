
/*  ----------------------------------------------------------------------- */
/*                  scripting                                               */
/*  ----------------------------------------------------------------------- */

#ifdef USE_GUILE
SCM safe_scheme_command_test(const char *cmd);
SCM safe_scheme_command(const std::string &scheme_command);
#else 
void safe_scheme_command(const std::string &scheme_command); /* do nothing */
#endif // USE_GUILE

#ifdef USE_PYTHON
void safe_python_command(const std::string &python_command); 
void safe_python_command_by_char_star(const char *python_command);
PyObject *py_clean_internal(PyObject *obj);
PyObject *safe_python_command_with_return(const std::string &python_cmd);
PyObject *safe_python_command_test(const char *cmd);
void safe_python_command_with_unsafe_thread(const char *cmd);
#endif // PYTHON
/*  Is this a repeat of something?  I don't know. */
void run_generic_script(const std::vector<std::string> &cmd_strings);

#ifdef USE_GUILE
SCM coot_has_python_p();
#endif
