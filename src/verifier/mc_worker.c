/*
 * mc_worker.c
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

/** @author Masato Gocho
 *  utilities for parallel/concurrent execution */

#include "mc.h"
#include "mc_worker.h"
#include "mc_generator.h"
#include "mc_explorer.h"
#include "state.h"
#include "statespace.h"
#include "error.h"
#include "runtime_status.h"

#include <limits.h>

/** -------------------------------------
 *  MC object
 */

static inline void lmn_mc_obj_init(LmnMCObj *mc, LmnWorker *w) {
  mc->type     = 0x00U;
  mc->obj      = NULL;
  mc->init     = NULL;
  mc->finalize = NULL;
  mc->owner    = w;
}


/** -------------------------------------
 *  LmnWorker
 */

static void lmn_worker_start(void *arg);
static void worker_TLS_init(unsigned int id);
static void worker_TLS_finalize(void);
static void worker_set_env(LmnWorker *w);


/* まっさらなLmnWorkerオブジェクトをmallocして返す */
inline LmnWorker *lmn_worker_make_minimal()
{
  LmnWorker *w = LMN_MALLOC(LmnWorker);

  w->id        = 0;
  w->f_safe    = 0x00U;
  w->f_exec    = 0x00U;
  w->f_end     = FALSE;
  w->f_end2    = FALSE;
  w->wait      = FALSE;
  w->states    = NULL;
  w->next      = NULL;
  w->start     = NULL;
  w->check     = NULL;
  w->invalid_seeds = NULL;
  w->cycles    = NULL;
  w->expand = 0;
  w->red =0;

  lmn_mc_obj_init(&w->generator, w);
  lmn_mc_obj_init(&w->explorer, w);

  return w;
}

/* LmnWorkerオブジェクトを初期化生成して返す */
LmnWorker *lmn_worker_make(StateSpace     ss,
                           unsigned long  id,
                           BOOL           flags)
{
  LmnWorker *w = lmn_worker_make_minimal();
  w->states = ss;
  w->id     = id;
  worker_flags_set(w, flags);
  worker_set_active(w);
  worker_set_white(w);

  return w;
}


void lmn_worker_free(LmnWorker *w)
{
  LMN_FREE(w);
}


/* Verification start */
static void lmn_worker_start(void *arg)
{
  LmnWorkerGroup *wp;
  LmnWorker *w;
  unsigned long id;

  w = (LmnWorker *)arg;
  wp = worker_group(w);
  id = worker_id(w);
  worker_TLS_init(id);

  mc_react_cxt_init(&worker_rc(w));

  if (worker_id(w) == LMN_PRIMARY_ID && mc_is_dump(worker_flags(w))) {
    StateSpace ss = worker_states(w);
    dump_state_data(statespace_init_state(ss), (LmnWord)ss->out, (LmnWord)NULL);
  }

  if (lmn_env.profile_level >= 1) profile_start_exec_thread();
  worker_start(w);

  if (!workers_are_exit(wp) && !workers_have_error(wp)) {
    if (worker_use_owcty(w)) {
      owcty_start(w);
    } else if (worker_use_map(w) && !worker_use_weak_map(w)) {
      map_iteration_start(w);
    }
    /* else: ND-mode etc */
  }

  if (lmn_env.profile_level >= 1) profile_finish_exec_thread();
  mc_react_cxt_destroy(&worker_rc(w));
  worker_TLS_finalize();
}

LmnWorker* worker_next_generator(LmnWorker* w)
{
  LmnWorker* next = worker_next(w);

  while(!worker_is_generator(next)) next = worker_next(next);
  return next; 
}

static void worker_TLS_init(unsigned int inc_id)
{
  /* 各スレッド毎に, 自分のTLS idを設定する */
  env_my_TLS_init(inc_id);
  lmn_thread_set_CPU_affinity(inc_id);
}


static void worker_TLS_finalize()
{
  env_my_TLS_finalize();
}


/** -----------------------------------------------------------
 *  Worker Group
 */

static void workers_gen(LmnWorkerGroup *owner,
                        unsigned int   w_num,
                        Automata       a,
                        Vector         *psyms,
                        BOOL           flags);
static void workers_free(LmnWorker **w, unsigned int w_num);
static void workers_ring_alignment(LmnWorkerGroup *wp);



/* 実行時オプションが増えたため,
 * Worker起動以前のこの時点で,
 * 組み合わせをサポートしていないオプションの整合性を取るなどを行う.
 * 実際に使用するオプションフラグを, lmn_env構造体からworker構造体にコピーする. */
static BOOL workers_flags_init(LmnWorkerGroup *wp, Automata property_a)
{
  BOOL flags = 0x00U;

  /* === 1. 出力フォーマット === */

  /* --- 1-1. GraphViz形式の使用 --- */
  if (lmn_env.output_format == DOT) {
    lmn_env.output_format = DEFAULT;
  }

  /* --- 1-2. インクリメンタルな状態の標準出力 --- */
  if (lmn_env.sp_dump_format == INCREMENTAL) {
    mc_set_dump(flags);
    lmn_env.dump = TRUE;
    if (lmn_env.mc_dump_format != CUI && lmn_env.mc_dump_format != LaViT) {
      lmn_fatal("unsupported incremental output format");
    }
    fprintf(stdout, "States\n");
  }

  /* --- 1-3. 並列処理中のサポート外オプション --- */
  if (lmn_env.core_num >= 2) {
    if (lmn_env.sp_dump_format == INCREMENTAL) {
#ifndef DEBUG
      lmn_fatal("unsupported combination incremental state dumper & parallelization.");
#endif
    }
#ifdef PROFILE
    if (lmn_env.optimize_hash_old) {
      lmn_fatal("unsupported combination optimized hash (old) & parallelization.");
    }
#endif
  }


  /* === 2. 状態空間構築オプション === */

  /* --- 2-1. 遷移オブジェクトの使用 --- */
  if (lmn_env.show_transition) {
    mc_set_trans(flags);
  }

  /* --- 2-2. バイト列エンコードを使用しない場合 -----
   * 対応していない全てのオプションフラグを下げる. */
  if (lmn_env.enable_compress_mem) {
    if (lmn_env.delta_mem) {
      mc_set_delta(flags);
    }
    mc_set_compress(flags);
  }
  else {
    lmn_env.delta_mem     = FALSE;
    lmn_env.optimize_hash = FALSE;
#ifdef PROFILE
    lmn_env.optimize_hash_old = FALSE;
#endif

    if (lmn_env.hyperlink) {
      lmn_fatal("no support: membrane isomorphis tester for hyper graph model");
    }
  }

  /* --- 2-3. バイト列エンコードと階層グラフ構造の一対一対応 --- */
  if (lmn_env.mem_enc) {
    mc_set_canonical(flags);
  }

  /* --- 2-4. Partial Order Reduction --- */
  if (lmn_env.enable_por || lmn_env.enable_por_old) {
    mc_set_por(flags);
  }

  /* --- 2-5. binstr compressor --- */
  if (lmn_env.z_compress) {
    lmn_env.d_compress = FALSE;
  }

  /* === 3. 状態空間探索(LTLモデル検査)オプション === */
  if (lmn_env.ltl) {
    if (!property_a) {
      lmn_env.ltl = FALSE;
      lmn_env.ltl_all = FALSE;
      wp->do_search = FALSE;
      wp->do_exhaustive = FALSE;
    } else {
      if (lmn_env.ltl_all) {
        wp->do_exhaustive = TRUE;
      }
      wp->do_search = TRUE;
    }

    if (lmn_env.enable_parallel) {
      wp->do_para_algo = TRUE;
    }
  }

  /* === 4. hypergraph ==== */
  if (lmn_env.hyperlink) {
    if (lmn_env.optimize_hash) {
      lmn_env.optimize_hash = FALSE;
    }

    if (lmn_env.delta_mem || lmn_env.mem_enc || !lmn_env.enable_compress_mem) {
      lmn_fatal("under constructions: delta/canonical membrane methods for hyper graph model");
    }
  }

  return flags;
}

LmnWorkerGroup *lmn_workergroup_make(Automata a, Vector *psyms, int thread_num)
{
  LmnWorkerGroup *wp;
  BOOL flags;

  wp = LMN_MALLOC(LmnWorkerGroup);

  /* worker pool の構築と初期設定 */
  wp->terminated  = FALSE;
  wp->worker_num  = thread_num;
  wp->stop        = FALSE;

  wp->do_search     = FALSE;
  wp->do_exhaustive = FALSE;
  wp->do_para_algo  = FALSE;
  wp->mc_exit       = FALSE;
  wp->error_exist   = FALSE;

  wp->opt_end_state = NULL;

#ifdef KWBT_OPT
  if (thread_num >= 2 && lmn_env.opt_mode != OPT_NONE) {
    wp->ewlock = ewlock_make(1U, DEFAULT_WLOCK_NUM);
  } else
#endif
    wp->ewlock = NULL;

  flags = workers_flags_init(wp, a);
#ifdef OPT_WORKERS_SYNC
  wp->synchronizer  = thread_num;
#else
  lmn_barrier_init(&wp->synchronizer, workers_entried_num(wp));
#endif
  workers_gen(wp, workers_entried_num(wp), a, psyms, flags);
  workers_ring_alignment(wp);

  return wp;
}


/* 後始末 */
void lmn_workergroup_free(LmnWorkerGroup *wp)
{
#ifndef OPT_WORKERS_SYNC
  lmn_barrier_destroy(&workers_synchronizer(wp));
#endif
  workers_free(wp->workers, workers_entried_num(wp));

  if (wp->ewlock) {
    ewlock_free(wp->ewlock);
  }
}


/* スレッドの起動 (MT-unsafe) */
void launch_lmn_workers(LmnWorkerGroup *wp)
{
  unsigned long i, core_num;

  core_num = workers_entried_num(wp);
  for (i = 0; i < core_num; i++) { /** start */
    if (i == LMN_PRIMARY_ID) continue;
    lmn_thread_create(&worker_pid(workers_get_worker(wp, i)),
                      lmn_worker_start,
                      workers_get_worker(wp, i));
  }
  if (lmn_env.profile_level >= 1) profile_start_exec();

  lmn_worker_start((void *)workers_get_worker(wp, LMN_PRIMARY_ID));

  if (lmn_env.profile_level >= 1) profile_finish_exec();
  for (i = 0; i < core_num; i++) {
    if (i == LMN_PRIMARY_ID) continue;
    lmn_thread_join(worker_pid(workers_get_worker(wp, i)));
  }
}


#define TERMINATION_CONDITION(W) \
 (worker_is_idle(W) && worker_is_white(W) && worker_check(w))


/* 全てのWorkerオブジェクトが実行を停止している場合に真を返す. */
BOOL lmn_workers_termination_detection_for_rings(LmnWorker *root)
{
  LmnWorkerGroup *wp;
  /** 概要:
   *  LmnWorkerは論理的に輪を形成しており, フラグチェックを輪に沿って実施する.
   *  is_activeかどうかをチェックする他にis_stealer, is_whiteもチェックする.
   *    is_white  :
   *       自idより小さいidのworkerにタスクを送信した場合の不正終了を判定するフラグ.
   *       (チェック済みのworkerに対するタスク送信が発生することで不正終了する)
   *       @see Dijkstra's Token Termination Detection (parallel computing)
   *    is_steaker:
   *       異なるWorkerからタスクを横取りしたこを示すフラグ.
   *       (チェック済のworkerが未チェックのworkerからタスクを横取りした場合に不正終了する)
   *
   *  Dijkstraの手法を一部参考にしてはいるが, Dijkstraの手法ではないため注意.
   */

  /* Primary Worker(id==LMN_PRIMARY_ID)が判定した検知結果をグローバル変数に書込む.
   * 他のWorkerはグローバル変数に書き込まれた終了フラグを読み出す. */
  wp = worker_group(root);
  if (worker_id(root) == LMN_PRIMARY_ID && !workers_are_terminated(wp)) {
    int i, n;
    BOOL ret;

    ret = TRUE;
    n   = workers_entried_num(wp);
    for (i = 0; i < n; i++) {
      LmnWorker *w = workers_get_worker(wp, i);
      ret = ret && TERMINATION_CONDITION(w);
      worker_set_white(w);
      if (!ret) return FALSE;
    }

    for (i = 0; i < n; i++) {
      LmnWorker *w = workers_get_worker(wp, i);
      ret = ret && !worker_is_stealer(w);
      worker_unset_stealer(w);
      if (!ret) return FALSE;
    }
    workers_set_terminated(wp);
  }

  return workers_are_terminated(wp);
}



/* 全てのWorkerオブジェクトで同期を取り, Primary Workerが関数funcを実行する.
 * 全てのWorkerがbarrierに到達したとき処理を再開する. */
void lmn_workers_synchronization(LmnWorker *me, void (*func)(LmnWorker *w))
{
  LmnWorkerGroup *wp = worker_group(me);

  lmn_barrier_wait(&workers_synchronizer(wp));
  if (worker_id(me) == LMN_PRIMARY_ID && func) {
    (*func)(me);
  }
  lmn_barrier_wait(&workers_synchronizer(wp));
}


/* 呼び出したスレッドのTLS IDに応じたLmnWorkerオブジェクトをWorkerPoolから返す */
LmnWorker *workers_get_my_worker(LmnWorkerGroup *wp)
{
  /* >>>>>>>> worker_TLS_init以前に使用してはならない <<<<<<< */
  return workers_get_worker(wp, env_my_thread_id());
}

/* WorkerPoolのid番目のLmnWorkerオブジェクトを返す */
LmnWorker *workers_get_worker(LmnWorkerGroup *wp, unsigned long id)
{
  if (id >= workers_entried_num(wp)) {
    return NULL;
  } else {
    return workers_get_entry(wp, id);
  }
}


static void workers_gen(LmnWorkerGroup *owner,
                        unsigned int   worker_num,
                        Automata       a,
                        Vector         *psyms,
                        BOOL           flags)
{
  unsigned int i;
  owner->workers = LMN_NALLOC(LmnWorker *, worker_num);
  for (i = 0; i < worker_num; i++) {
    LmnWorker *w;
    StateSpace states;

    if (i == 0) {
      states = worker_num > 1 ? statespace_make_for_parallel(worker_num, a, psyms)
                              : statespace_make(a, psyms);
    } else {
      states = worker_states(workers_get_worker(owner, 0));
    }

    w = lmn_worker_make(states, i, flags);
    owner->workers[i] = w;
    w->group = owner;

    if (owner->do_search) {
      w->invalid_seeds = vec_make(4);
      w->cycles = vec_make(4);
    }

    /* アルゴリズムの割り当てと初期化 */
    worker_set_env(w);
    worker_init(w);
  }
}


static void workers_free(LmnWorker **pool, unsigned int worker_num)
{
  unsigned int i, j;
  for (i = 0; i < worker_num; i++) {
    LmnWorker *w = pool[i];
    if (worker_group(w)->do_search) {
      vec_free(w->invalid_seeds);

      for (j = 0; j < vec_num(w->cycles); j++) {
        vec_free((Vector *)vec_get(w->cycles, j));
      }
      vec_free(w->cycles);
    }

    if (i == 0) {
      statespace_free(worker_states(w));
    }

    worker_finalize(w);
    lmn_worker_free(w);
  }
  LMN_FREE(pool);
}


static void workers_ring_alignment(LmnWorkerGroup *wp)
{
  unsigned int i, n;
  n = workers_entried_num(wp);
  for (i = 0; i < n; i++) {
    LmnWorker *w, *next;
    w    = workers_get_worker(wp, i);
    next = workers_get_worker(wp, (i + 1) % n);
    w->next = next;
  }
}


/* workerの実行アルゴリズムの割当を行う */
static void worker_set_env(LmnWorker *w)
{
  if (!lmn_env.nd) {
    lmn_fatal("UnExepcted Yet.");
  }

  if (lmn_env.enable_parallel)        worker_set_parallel(w);
  if (lmn_env.optimize_loadbalancing) worker_set_dynamic_lb(w);

  if (!lmn_env.bfs) { /* Depth First Search */
    dfs_env_set(w);
  } else {            /* Breadth First Search */
    bfs_env_set(w);
  }

  if (lmn_env.ltl) {
    if (lmn_env.enable_owcty) {
      owcty_env_set(w);
    } else if (lmn_env.enable_map) {
      map_env_set(w);
    } else if(lmn_env.enable_mapndfs) {
      mapndfs_env_set(w);
    } else if(lmn_env.enable_mcndfs) {
      mcndfs_env_set(w);
    } else if (lmn_env.enable_bledge || worker_use_lsync(w) || worker_on_mc_bfs(w)) {
      bledge_env_set(w);
    }

    /* 特定のアルゴリズムが指定されていない場合のデフォルト動作 */
    if (worker_ltl_none(w)) {
      if (worker_on_parallel(w) || worker_on_mc_bfs(w)) {
        /* 並列アルゴリズムを使用している or BFSの場合のデフォルト */
        owcty_env_set(w);
      } else {
        ndfs_env_set(w);
      }
    }


    if (worker_use_owcty(w)) {
      lmn_env.enable_owcty = TRUE;
    } else if (worker_use_ble(w)) {
      lmn_env.enable_bledge = TRUE;
    }

    if (!worker_use_opt_scc(w)) {
      lmn_env.prop_scc_driven = FALSE;
    }
  }
}


LmnCost workers_opt_cost(LmnWorkerGroup *wp)
{
  LmnCost cost;
  State *opt = workers_opt_end_state(wp);
  if (!opt) {
    cost = lmn_env.opt_mode == OPT_MINIMIZE ? ULONG_MAX : 0;
  } else {
    cost = state_cost(opt);
  }
  return cost;
}

/* 最適コストを可能ならば更新する
 * f==true: minimize
 * f==false: maximize */
void lmn_update_opt_cost(LmnWorkerGroup *wp, State *new_s, BOOL f)
{
  if (env_threads_num() >= 2) workers_opt_end_lock(wp);
  if ((f && workers_opt_cost(wp) > state_cost(new_s)) ||
      (!f && workers_opt_cost(wp) < state_cost(new_s))) {
    workers_opt_end_state(wp) = new_s;
  }
  if (env_threads_num() >= 2) workers_opt_end_unlock(wp);
}

