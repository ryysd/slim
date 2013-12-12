/*
 * so.h
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group <lmntal@ueda.info.waseda.ac.jp>
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *    3. Neither the name of the Ueda Laboratory LMNtal Groupy LMNtal
 *       Group nor the names of its contributors may be used to
 *       endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: so.h,v 1.00 2009/10/01 18:18:00 riki Exp $
 */

/* このファイルはトランスレータから出力された.cファイルの中で必要なものをすべて含む */
/* .cの中でのみ必要な情報はここに直接書く */

#ifndef SO_H
#define SO_H

#include "lmntal.h"
#include "rule.h"
#include "functor.h"
#include "translate.h"
#include "load.h"
#include "symbol.h"
#include "react_context.h"
#include "slim_header/memstack.h"
#include "special_atom.h"
#include "error.h"
#include "task.h"
#include "mc.h"
#include "visitlog.h"

#ifdef PROFILE
#  include "runtime_status.h"
#endif

/* TR_GSID(x) translate global symbol id xのグローバルidを得る (定義に出力ファイル名を含むため.c内で出力) */
/* TR_GFID(x) translate global functor id xのグローバルidを得る (定義に出力ファイル名を含むため.c内で出力) */
/* TR_GRID(x) translate global ruleset id xのグローバルidを得る (定義に出力ファイル名を含むため.c内で出力) */
/* インタプリタ用の定義では変換は必要ないため TR_G*ID(x) = x となる */

#define LINKED_ATOM(x) wt(rc, x)
#define LINKED_ATTR(x) at(rc, x)

#define TR_INSTR_ALLOCLINK(rc, link, atom, n)                           \
  do {                                                                  \
    if (LMN_ATTR_IS_DATA(at(rc, atom))) {                               \
      warry_set(rc, link, wt(rc, atom), at(rc, atom), TT_ATOM);         \
    } else { /* link to atom */                                         \
      warry_set(rc, link, LMN_SATOM(wt(rc, atom)), LMN_ATTR_MAKE_LINK(n), TT_ATOM); \
    }                                                                   \
  } while(0)

/* @see INSTR_SPEC in task.c */
#define TR_INSTR_SPEC(rc, size)                                         \
  do {                                                                  \
    if(size > warry_size(rc)){                                          \
      lmn_register_extend(rc, round2up(size));                          \
    }                                                                   \
    warry_use_size_set(rc, size);                                       \
  } while(0)

#define TR_INSTR_UNIFYLINKS(rc, link1, link2, mem)                      \
  do{                                                                   \
      LmnLinkAttr attr1 = LINKED_ATTR(link1);                           \
      LmnLinkAttr attr2 = LINKED_ATTR(link2);                           \
                                                                        \
      if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr1)) {                         \
        if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)) { /* 1, 2 are data */   \
          lmn_mem_link_data_atoms((LmnMembrane *)wt(rc, mem),           \
                                  wt(rc, link1), at(rc, link1), LINKED_ATOM(link2), attr2);\
        }                                                               \
        else { /* 1 is data */                                          \
          LMN_SATOM_SET_LINK(LINKED_ATOM(link2), LMN_ATTR_GET_VALUE(attr2), LINKED_ATOM(link1)); \
          LMN_SATOM_SET_ATTR(LINKED_ATOM(link2), LMN_ATTR_GET_VALUE(attr2), attr1); \
        }                                                               \
      }                                                                 \
      else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr2)) { /* 2 is data */    \
        LMN_SATOM_SET_LINK(LINKED_ATOM(link1), LMN_ATTR_GET_VALUE(attr1), LINKED_ATOM(link2)); \
        LMN_SATOM_SET_ATTR(LINKED_ATOM(link1), LMN_ATTR_GET_VALUE(attr1), attr2); \
      }                                                                 \
      else { /* 1, 2 are symbol atom */                                 \
                                                                        \
        if (LMN_ATTR_IS_EX(attr1)) {                                    \
          if (LMN_ATTR_IS_EX(attr2)) { /* 1, 2 are ex */                \
            lmn_newlink_with_ex((LmnMembrane *)wt(rc, mem),             \
                                LMN_SATOM(LINKED_ATOM(link1)), attr1, 0, \
                                LMN_SATOM(LINKED_ATOM(link2)), attr2, 0);\
          } else { /* 1 is ex */                                         \
            lmn_newlink_with_ex((LmnMembrane *)wt(rc, mem),              \
                                LMN_SATOM(LINKED_ATOM(link1)), attr1, 0, \
                                LMN_SATOM(LINKED_ATOM(link2)), attr2, attr2);\
          }                                                                   \
        } else if (LMN_ATTR_IS_EX(attr2)) { /* 2 is ex */                     \
          lmn_newlink_with_ex((LmnMembrane *)wt(rc, mem),                     \
                              LMN_SATOM(LINKED_ATOM(link1)), attr1, attr1,    \
                              LMN_SATOM(LINKED_ATOM(link2)), attr2, 0);       \
        } else {                                                              \
          LMN_SATOM_SET_LINK(LINKED_ATOM(link1), LMN_ATTR_GET_VALUE(attr1), LINKED_ATOM(link2)); \
          LMN_SATOM_SET_LINK(LINKED_ATOM(link2), LMN_ATTR_GET_VALUE(attr2), LINKED_ATOM(link1)); \
          LMN_SATOM_SET_ATTR(LINKED_ATOM(link1), LMN_ATTR_GET_VALUE(attr1), attr2);              \
          LMN_SATOM_SET_ATTR(LINKED_ATOM(link2), LMN_ATTR_GET_VALUE(attr2), attr1);              \
        }                                                               \
                                                                        \
      }                                                                 \
  } while(0)

#define TR_INSTR_RELINK(rc, atom1,pos1,atom2,pos2,memi)                     \
  do{                                                                       \
    LmnSAtom ap;                                                            \
    LmnByte attr;                                                           \
    ap = LMN_SATOM(LMN_SATOM_GET_LINK(wt(rc, atom2), pos2));                \
    attr = LMN_SATOM_GET_ATTR(wt(rc, atom2), pos2);                         \
                                                                            \
    if(LMN_ATTR_IS_DATA_WITHOUT_EX(at(rc, atom1)) && LMN_ATTR_IS_DATA_WITHOUT_EX(attr)) {\
      fprintf(stderr, "Two data atoms are connected each other.\n");        \
    }                                                                       \
    else if (LMN_ATTR_IS_DATA_WITHOUT_EX(at(rc, atom1))) {                  \
      LMN_SATOM_SET_LINK(ap, attr, wt(rc, atom1));                          \
      LMN_SATOM_SET_ATTR(ap, attr, at(rc, atom1));                          \
    }                                                                       \
    else if (LMN_ATTR_IS_DATA_WITHOUT_EX(attr)) {                           \
      LMN_SATOM_SET_LINK(LMN_SATOM(wt(rc, atom1)), pos1, ap);               \
      LMN_SATOM_SET_ATTR(LMN_SATOM(wt(rc, atom1)), pos1, attr);             \
    }                                                                       \
    else if (!LMN_ATTR_IS_EX(at(rc, atom1)) && !LMN_ATTR_IS_EX(attr)) {     \
      LMN_SATOM_SET_LINK(ap, attr, wt(rc, atom1));                          \
      LMN_SATOM_SET_ATTR(ap, attr, pos1);                                   \
      LMN_SATOM_SET_LINK(LMN_SATOM(wt(rc, atom1)), pos1, ap);               \
      LMN_SATOM_SET_ATTR(LMN_SATOM(wt(rc, atom1)), pos1, attr);             \
    } else if (LMN_ATTR_IS_EX(at(rc, atom1))) {                             \
      lmn_newlink_with_ex((LmnMembrane *)wt(rc, memi),                      \
                          LMN_SATOM(wt(rc, atom1)),                         \
                          at(rc, atom1),                                    \
                          pos1,                                             \
                          ap,                                               \
                          0,                                                \
                          attr);                                            \
    } else {                                                                \
      lmn_newlink_with_ex((LmnMembrane *)wt(rc, memi),                      \
                          LMN_SATOM(wt(rc, atom1)),                         \
                          at(rc, atom1),                                    \
                          pos1,                                             \
                          ap,                                               \
                          attr,                                             \
                          0);                                               \
                                                                            \
    }                                                                       \
  } while(0)

#define TR_INSTR_COPYRULES(rc, destmemi, srcmemi)                          \
  do {                                                                     \
    unsigned int i;                                                        \
    struct Vector *v;                                                      \
    v = &((LmnMembrane *)wt(rc, srcmemi))->rulesets;                       \
    for (i = 0; i< v->num; i++) {                                          \
      LmnRuleSet cp = lmn_ruleset_copy((LmnRuleSet)vec_get(v, i));         \
      lmn_mem_add_ruleset((LmnMembrane *)wt(rc, destmemi), cp);            \
      if (RC_GET_MODE(rc, REACT_ATOMIC)) {                                 \
        lmn_ruleset_invalidate_atomic(cp);                                 \
      }                                                                    \
    }                                                                      \
  } while(0)

#define TR_INSTR_LOOKUPLINK(rc, destlinki, tbli, srclinki)                 \
  do {                                                                     \
    at_set(rc, destlinki, LINKED_ATTR(srclinki));                          \
    tt_set(rc, destlinki, TT_ATOM);                                        \
    if (LMN_ATTR_IS_DATA(LINKED_ATTR(srclinki))) {                         \
      wt_set(rc, destlinki, LINKED_ATOM(srclinki));                        \
    } else { /* symbol atom */                                             \
      ProcessTbl ht = (ProcessTbl)wt(rc, tbli);                            \
      proc_tbl_get_by_atom(ht, LMN_SATOM(LINKED_ATOM(srclinki)), &wt(rc, destlinki));\
    }                                                                      \
  } while(0)

#define TR_INSTR_DELETECONNECTORS(srcset, srcmap)                          \
  do {                                                                     \
    HashSet *delset;                                                       \
    ProcessTbl delmap;                                                     \
    HashSetIterator it;                                                    \
    delset = (HashSet *)wt(rc, srcset);                                    \
    delmap = (ProcessTbl)wt(rc, srcmap);                                   \
                                                                           \
    for (it = hashset_iterator(delset); !hashsetiter_isend(&it); hashsetiter_next(&it)) {\
      LmnSAtom orig, copy;                                                 \
      LmnWord t;                                                           \
                                                                           \
      orig = LMN_SATOM(hashsetiter_entry(&it));                            \
      t = 0; /* warningを黙らす */                                           \
      proc_tbl_get_by_atom(delmap, orig, &t);                              \
      copy = LMN_SATOM(t);                                                 \
      lmn_mem_unify_symbol_atom_args(orig, 0, orig, 1);                    \
      lmn_mem_unify_symbol_atom_args(copy, 0, copy, 1);                    \
                                                                           \
      lmn_delete_atom(orig);                                               \
      lmn_delete_atom(copy);                                               \
    }                                                                      \
                                                                           \
    if (delmap) proc_tbl_free(delmap);                                     \
  } while (0)

#define TR_INSTR_DEREFFUNC(rc, funci, atomi, pos)                          \
  do {                                                                     \
    LmnLinkAttr attr = LMN_SATOM_GET_ATTR(LMN_SATOM(wt(rc, atomi)), pos);  \
    if (LMN_ATTR_IS_DATA(attr)) {                                          \
      warry_set(rc, funci,                                                 \
                LMN_SATOM_GET_LINK(LMN_SATOM(wt(rc, atomi)), pos),         \
                attr,                                                      \
                TT_OTHER);                                                 \
    }                                                                      \
    else { /* symbol atom */                                               \
    warry_set(rc, funci,                                                   \
              LMN_SATOM_GET_FUNCTOR(LMN_SATOM_GET_LINK(LMN_SATOM(wt(rc, atomi)), pos)),\
              attr,                                                        \
              TT_OTHER);                                                   \
    }                                                                      \
  } while(0)

void tr_instr_commit_ready(LmnReactCxt       *rc,
                           LmnRule           rule,
                           lmn_interned_str  rule_name,
                           LmnLineNum        line_num,
                           LmnMembrane       **ptmp_global_root,
                           LmnRegister       **V,
                           unsigned int      *org_next_id);
BOOL tr_instr_commit_finish(LmnReactCxt      *rc,
                            LmnRule          rule,
                            lmn_interned_str rule_name,
                            LmnLineNum       line_num,
                            LmnMembrane      **ptmp_global_root,
                            LmnRegister      **p_v_tmp,
                            unsigned int     warry_use_org,
                            unsigned int     warry_size_org);
BOOL tr_instr_jump(LmnTranslated   f,
                   LmnReactCxt     *rc,
                   LmnMembrane     *thisisrootmembutnotused,
                   LmnRule         rule,
                   int             newid_num,
                   const int       *newid);
#endif
