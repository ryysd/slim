/*
 * nc_parser.y - parse never claim in Promela
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group
 *                                          <lmntal@ueda.info.waseda.ac.jp>
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
 *    3. Neither the name of the Ueda Laboratory LMNtal Group nor the
 *       names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior
 *       written permission.
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
 * $Id$
 */

%{
#include <math.h>
#include <stdio.h>
#include "st.h"
#include "automata.h"
#include "vector.h"
%}

%pure-parser
%locations
%parse-param {yyscan_t scanner}
%parse-param {Automata automata}
%lex-param {yyscan_t scanner}

%union {
  int _int;
  char *str;
  struct AutomataState *state;
  struct AutomataTransition *transition;
  struct Vector *vector;
  struct PLFormula *PLFormula;
}

%token <str> SYMBOL
%token <_int> NUMBER
%token COLON COLON_COLON  SEMI_COLON LBRACE RBRACE LPAREN RPAREN
%token AND OR IMPLICATION NEGATION EQUIVALENCE
%token ALWAYS EVENTUALLY UNTIL RELEASE NEXT
%token KW_GOTO KW_IF KW_FI KW_SKIP KW_TRUE KW_FALSE

%token _EOF 0;

%start never_claim;
%type <vector> states;
%type <state> state;
%type <vector> transitions;
%type <transition> transition;
%type <_int> next;
%type <PLFormula> propositional_term;
%type <PLFormula> propositional_factor;
%type <PLFormula> propositional_exp;

%{
#include "nc_lexer.h"
static void ncerror (YYLTYPE*, yyscan_t, Automata, char *);
%}

%% /* Grammar rules and actions follow.  */

never_claim:
  SYMBOL LBRACE states RBRACE _EOF {
    unsigned int i;
    Vector *states = $3;
    AutomataState s;

    s = (AutomataState)vec_get(states, 0);
    automata_set_init_state(automata, atmstate_id(s));
    for (i = 0; i < vec_num(states); i++) {
      automata_add_state(automata, (AutomataState)vec_get(states, i));
    }
    free($1);
    vec_free(states);
  }
;

states:
/* empty */ { $$ = vec_make(32); }
| states state  { vec_push($1, (vec_data_t)$2); $$ = $1; }
;

state:
  SYMBOL COLON KW_IF transitions KW_FI SEMI_COLON {
    unsigned int i, cur;
    AutomataState s;
    BOOL is_accept_state = FALSE;

    if (strstr($1, "accept") != NULL) is_accept_state = TRUE;
    cur = automata_state_id(automata, $1);
    s = atmstate_make(cur, is_accept_state, 0);
    for (i = 0; i < vec_num($4); i++) {
      atmstate_add_transition(s, (AutomataTransition)vec_get($4, i));
    }
    $$ = s;

    free($1);
    vec_free($4);
  }
| SYMBOL COLON KW_SKIP {
    AutomataState s;
    unsigned int cur;
    BOOL is_accept_state = FALSE;

    if (strstr($1, "accept") != NULL) is_accept_state = TRUE;
    cur = automata_state_id(automata, $1);
    s = atmstate_make(cur, is_accept_state, 1);
    atmstate_add_transition(s, atm_transition_make(cur, true_node_make()));
    $$ = s;
    free($1);
  }
;

transitions:
  /* empty */ { $$ = vec_make(32); }
| transitions transition { vec_push($1, (vec_data_t)$2); $$ = $1; }
;

transition:
  COLON_COLON propositional_exp IMPLICATION next {
    $$ = atm_transition_make($4, $2);
  }
| COLON_COLON LPAREN NUMBER RPAREN IMPLICATION next {
    $$ = atm_transition_make($6, true_node_make());
  }
;

next:
  KW_GOTO SYMBOL { $$ = automata_state_id(automata, $2); free($2); }
;

propositional_exp:
  propositional_term AND propositional_exp { $$ = and_node_make($1, $3); }
| propositional_term OR propositional_exp { $$ = or_node_make($1, $3); }
| propositional_term { $$ = $1; }
;

propositional_term:
  NEGATION propositional_term { $$ = negation_node_make($2); }
| propositional_factor { $$ = $1; }
;

propositional_factor:
  LPAREN propositional_exp RPAREN { $$ = $2; }
| SYMBOL { $$ = sym_node_make(automata_propsym_to_id(automata, $1)); free($1); }
| KW_TRUE { $$ = true_node_make(); }
| KW_FALSE { $$ = false_node_make(); }
;

%%

#include "st.h"

/* Called by yyparse on error.  */
void ncerror (YYLTYPE *loc, yyscan_t scanner, Automata a, char *s)
{
  fprintf(stderr, "never claim parser: error %s line: %d\n", s, ncget_lineno(scanner));
  exit(EXIT_FAILURE);
}
