/* ----------------------------------------------------------------------------- 
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * fragment.c
 *
 * This file manages named code fragments.  Code fragments are typically
 * used to hold helper-code that may or may not be included in the wrapper
 * file (depending on what features are actually used in the interface).
 *
 * By using fragments, it's possible to greatly reduce the amount of
 * wrapper code and to generate cleaner wrapper files. 
 * ----------------------------------------------------------------------------- */

char cvsroot_fragment_c[] = "$Header$";

#include "swig.h"
#include "swigkeys.h"
#include "swigwarn.h"

static Hash *fragments = 0;
static Hash *looking_fragments = 0;
static int debug = 0;


/* -----------------------------------------------------------------------------
 * Swig_fragment_register()
 *
 * Add a fragment. Use the original Node*, so, if something needs to be
 * changed, lang.cxx doesn't nedd to be touched again.
 * ----------------------------------------------------------------------------- */

void Swig_fragment_register(Node *fragment) {
  if (Getattr(fragment, k_emitonly)) {
    Swig_fragment_emit(fragment);
    return;
  } else {
    String *name = Copy(Getattr(fragment, k_value));
    String *type = Getattr(fragment, k_type);
    if (type) {
      SwigType *rtype = SwigType_typedef_resolve_all(type);
      String *mangle = Swig_string_mangle(type);
      Append(name, mangle);
      Delete(mangle);
      Delete(rtype);
      if (debug)
	Printf(stdout, "register fragment %s %s\n", name, type);
    }
    if (!fragments) {
      fragments = NewHash();
    }
    if (!Getattr(fragments, name)) {
      String *section = Copy(Getattr(fragment, k_section));
      String *ccode = Copy(Getattr(fragment, k_code));
      Hash *kwargs = Getattr(fragment, k_kwargs);
      Setmeta(ccode, k_section, section);
      if (kwargs) {
	Setmeta(ccode, k_kwargs, kwargs);
      }
      Setattr(fragments, name, ccode);
      if (debug)
	Printf(stdout, "registering fragment %s %s\n", name, section);
      Delete(section);
      Delete(ccode);
    }
    Delete(name);
  }
}

/* -----------------------------------------------------------------------------
 * Swig_fragment_emit()
 *
 * Emit a fragment
 * ----------------------------------------------------------------------------- */

static
char *char_index(char *str, char c) {
  while (*str && (c != *str))
    ++str;
  return (c == *str) ? str : 0;
}

void Swig_fragment_emit(Node *n) {
  String *code;
  char *pc, *tok;
  String *t;
  String *mangle = 0;
  String *name = 0;
  String *type = 0;

  if (!fragments) {
    Swig_warning(WARN_FRAGMENT_NOT_FOUND, Getfile(n), Getline(n), "Fragment '%s' not found.\n", name);
    return;
  }


  name = Getattr(n, k_value);
  if (!name) {
    name = n;
  }
  type = Getattr(n, k_type);
  if (type) {
    mangle = Swig_string_mangle(type);
  }

  if (debug)
    Printf(stdout, "looking fragment %s %s\n", name, type);
  t = Copy(name);
  tok = Char(t);
  pc = char_index(tok, ',');
  if (pc)
    *pc = 0;
  while (tok) {
    String *name = NewString(tok);
    if (mangle)
      Append(name, mangle);
    if (looking_fragments && Getattr(looking_fragments, name)) {
      return;
    }
    code = Getattr(fragments, name);
    if (debug)
      Printf(stdout, "looking subfragment %s\n", name);
    if (code && (Strcmp(code, k_ignore) != 0)) {
      String *section = Getmeta(code, k_section);
      Hash *nn = Getmeta(code, k_kwargs);
      if (!looking_fragments)
	looking_fragments = NewHash();
      Setattr(looking_fragments, name, "1");
      while (nn) {
	if (Equal(Getattr(nn, k_name), k_fragment)) {
	  if (debug)
	    Printf(stdout, "emitting fragment %s %s\n", nn, type);
	  Setfile(nn, Getfile(n));
	  Setline(nn, Getline(n));
	  Swig_fragment_emit(nn);
	}
	nn = nextSibling(nn);
      }
      if (section) {
	File *f = Swig_filebyname(section);
	if (!f) {
	  Swig_error(Getfile(code), Getline(code), "Bad section '%s' for code fragment '%s'\n", section, name);
	} else {
	  if (debug)
	    Printf(stdout, "emitting subfragment %s %s\n", name, section);
	  if (debug)
	    Printf(f, "/* begin fragment %s */\n", name);
	  Printf(f, "%s\n", code);
	  if (debug)
	    Printf(f, "/* end fragment %s */\n\n", name);
	  Setattr(fragments, name, k_ignore);
	  Delattr(looking_fragments, name);
	}
      }
    } else if (!code && type) {
      SwigType *rtype = SwigType_typedef_resolve_all(type);
      if (!Equal(type, rtype)) {
	String *name = Copy(Getattr(n, k_value));
	String *mangle = Swig_string_mangle(type);
	Append(name, mangle);
	Setfile(name, Getfile(n));
	Setline(name, Getline(n));
	Swig_fragment_emit(name);
	Delete(mangle);
	Delete(name);
      }
      Delete(rtype);
    }

    if (!code) {
      Swig_warning(WARN_FRAGMENT_NOT_FOUND, Getfile(n), Getline(n), "Fragment '%s' not found.\n", name);
    }
    tok = pc ? pc + 1 : 0;
    if (tok) {
      pc = char_index(tok, ',');
      if (pc)
	*pc = 0;
    }
    Delete(name);
  }
  Delete(t);
}
