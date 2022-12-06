#include<stdio.h>
#ifdef DEBUG
  #define DEBUG_FUNC_BEGIN printf("Entering %s\n",__FUNCTION__);
  #define DEBUG_FUNC_EXIT printf("Exiting %s\n",__FUNCTION__);
#else
  #define DEBUG_FUNC_BEGIN
  #define DEBUG_FUNC_EXIT
#endif

void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit (void *, void *) __attribute__((no_instrument_function));

int depth = -1;

void __cyg_profile_func_enter (void *func,  void *caller)
{
  int n;
  depth++;
  for (n = 0; n < depth; n++)
    printf ("  ");
  printf ("-> %p\n", func);
}

void __cyg_profile_func_exit (void *func, void *caller)
{
  int n;
  for (n = 0; n < depth; n++)
    printf ("  ");
  printf ("<- %p\n", func);
  depth--;
}

void bar(void)
{
DEBUG_FUNC_BEGIN
DEBUG_FUNC_EXIT
}

void foo (void)
{
 int x;
DEBUG_FUNC_BEGIN
 for (x = 0; x < 3; x++)
   bar ();
DEBUG_FUNC_EXIT
}

int main (int argc, char *argv[])
{
  int x;
DEBUG_FUNC_BEGIN
  for (x = 0; x < 3; x++) foo ();
  printf("Success\n");
DEBUG_FUNC_EXIT
  return 0;
}
