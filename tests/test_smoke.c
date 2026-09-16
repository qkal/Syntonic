/* Off-screen smoke test: the library links and the umbrella header is usable
 * from a plain C translation unit. */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "syntonic/syntonic.h"

int main(void) {
  const char *version = ns_version_string();
  assert(version != NULL);
  assert(strlen(version) > 0);
  printf("syntonic %s\n", version);
  return 0;
}
