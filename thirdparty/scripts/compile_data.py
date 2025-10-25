#!/usr/bin/env python3
import csv, re, sys, os
from pathlib import Path

# ---------- helpers ----------
def esc_c(s: str) -> str:
    # replace backslashes first, then quotes, then newlines
    return s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')

def norm_id(s: str) -> str:
    return re.sub(r'[^a-zA-Z0-9_]', '_', s.strip())

def parse_scenes(path: Path):
    scenes = []
    cur = {}
    with path.open('r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith('SCENE:'):
                if cur:
                    scenes.append(cur)
                    cur = {}
                cur['scene_id'] = line.split(':',1)[1].strip()
                # defaults
                cur['type'] = 'normal'
                cur['text'] = ''
                cur['next'] = ''
                cur['trigger_quiz'] = ''
                cur['question_id'] = ''
                cur['bg'] = '0'
                cur['music'] = '0'
            else:
                k, v = line.split(':', 1)
                k = k.strip()
                v = v.strip()
                if k == 'text':
                    # pipes are newlines
                    v = v.replace('|', '\n')
                cur[k] = v
        if cur:
            scenes.append(cur)
    return scenes

def parse_questions_csv(path: Path):
    rows = []
    with path.open('r', encoding='utf-8') as f:
        r = csv.DictReader(f)
        for row in r:
            rows.append(row)
    return rows

def parse_quizzes(path: Path):
    quizzes = []
    cur = {}
    with path.open('r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith('QUIZ:'):
                if cur:
                    quizzes.append(cur)
                    cur = {}
                cur['quiz_id'] = line.split(':',1)[1].strip()
                cur['name'] = ''
                cur['wrong_limit'] = '0'
                cur['questions'] = '0'
                cur['categories'] = []
            else:
                k, v = line.split(':', 1)
                k = k.strip()
                v = v.strip()
                if k == 'categories':
                    cur['categories'] = [c.strip() for c in v.split(',') if c.strip()]
                elif k == 'wrong_limit' or k == 'questions':
                    cur[k] = v
                elif k == 'name':
                    cur['name'] = v
                else:
                    cur[k] = v
        if cur:
            quizzes.append(cur)
    return quizzes

def build_categories(questions_rows, quizzes):
    names = set()
    for q in questions_rows:
        names.add(q['category'].strip())
    for qu in quizzes:
        for c in qu['categories']:
            names.add(c)
    names = [n for n in sorted(names) if n]
    index = {n:i for i,n in enumerate(names)}
    return names, index

def index_by_id(items, key):
    idx = {}
    for i, it in enumerate(items):
        idx[it[key]] = i
    return idx

# ---------- main ----------
def main():
    if len(sys.argv) != 5:
        print("Usage: build_data.py <scenes.txt> <questions.csv> <quizzes.txt> <out_c_path>")
        sys.exit(1)

    scenes_path   = Path(sys.argv[1])
    questions_path= Path(sys.argv[2])
    quizzes_path  = Path(sys.argv[3])
    out_c         = Path(sys.argv[4])

    scenes = parse_scenes(scenes_path)
    questions_rows = parse_questions_csv(questions_path)
    quizzes = parse_quizzes(quizzes_path)

    # categories
    cat_names, cat_index = build_categories(questions_rows, quizzes)

    # map IDs to indices
    q_by_id = index_by_id(questions_rows, 'id')
    scene_by_id = index_by_id(scenes, 'scene_id')
    quiz_by_id = index_by_id(quizzes, 'quiz_id')

    # prepare questions (normalize answers -> up to 4)
    # CSV headers: id,category,question,answer_a,answer_b,answer_c,correct
    def correct_to_idx(v: str) -> int:
        v = v.strip().lower()
        if v in ('a','b','c','d'):
            return 'abcd'.index(v)
        # fallback if a number slipped in
        try:
            i = int(v)
            return max(0, min(3, i))
        except:
            return 0

    # build C
    lines = []
    emit = lines.append

    emit('#include <genesis.h>')
    emit('#include "data_types.h"')
    emit('#include "data_load.h"')
    emit('')

    # Category names
    emit('// ---- Categories ----')
    emit('const char * const CATEGORY_NAMES[] = {')
    for n in cat_names:
        emit(f'  "{esc_c(n)}",')
    emit('};')
    emit(f'const u16 CATEGORY_COUNT = {len(cat_names)};')
    emit('')

    # Questions
    emit('// ---- Questions ----')
    emit('static const Question QUESTIONS_DATA[] = {')
    for i, row in enumerate(questions_rows):
        cat_id = cat_index[row['category'].strip()]
        qtxt = esc_c(row['question'])
        a = esc_c(row.get('answer_a',''))
        b = esc_c(row.get('answer_b',''))
        c = esc_c(row.get('answer_c',''))
        d = ""  # not provided in your CSV; reserve for future
        corr = correct_to_idx(row.get('correct','a'))
        emit('  {')
        emit(f'    .id = {i},')
        emit(f'    .category_id = {cat_id},')
        emit(f'    .question = "{qtxt}",')
        emit(f'    .answerA = "{a}",')
        emit(f'    .answerB = "{b}",')
        emit(f'    .answerC = "{c}",')
        emit(f'    .correct = {corr},')
        emit('  },')
    emit('};')
    emit(f'const Question * const QUESTIONS = QUESTIONS_DATA;')
    emit(f'const u16 QUESTIONS_COUNT = {len(questions_rows)};')
    emit('')

    # Scenes
    # types map
    type_map = {'normal':'SCENE_TYPE_NORMAL', 'quiz_trigger':'SCENE_TYPE_QUIZ_TRIGGER',
                'good_ending':'SCENE_TYPE_GOOD_ENDING', 'bad_ending':'SCENE_TYPE_BAD_ENDING'}

    emit('// ---- Scenes ----')
    emit('static const Scene SCENES_DATA[] = {')
    for i, s in enumerate(scenes):
        stype = type_map.get(s.get('type','normal').strip(), 'SCENE_NORMAL')
        text = esc_c(s.get('text',''))
        # resolve optional links
        next_scene = s.get('next','').strip()
        trig_quiz  = s.get('trigger_quiz','').strip()
        question_id= s.get('question_id','').strip()

        next_idx = scene_by_id.get(next_scene, -1) if next_scene else -1
        trig_idx = quiz_by_id.get(trig_quiz, -1) if trig_quiz else -1
        q_idx    = q_by_id.get(question_id, -1) if question_id else -1

        bg = int(s.get('bg','0') or 0)
        music = int(s.get('music','0') or 0)

        emit('  {')
        emit(f'    .id = {i},')
        emit(f'    .type = {stype},')
        emit(f'    .text = "{text}",')
        emit(f'    .nextScene = {next_idx},')
        emit(f'    .triggerQuiz = {trig_idx},')
        emit(f'    .questionId = {q_idx},')
        emit(f'    .bg = {bg}, .music = {music},')
        emit('  },')
    emit('};')
    emit(f'const Scene * const SCENES = SCENES_DATA;')
    emit(f'const u16 SCENES_COUNT = {len(scenes)};')
    emit('')

    # Quizzes
    emit('// ---- Quizzes ----')
    # Flatten each quiz's category list into its own const array
    for i, qz in enumerate(quizzes):
        arrname = f'_QUIZ_CATS_{i}'
        cats = [cat_index[c] for c in qz['categories']]
        emit(f'static const u16 {arrname}[] = {{ ' + ', '.join(str(c) for c in cats) + ' };')

    emit('static const Quiz QUIZZES_DATA[] = {')
    for i, qz in enumerate(quizzes):
        name = esc_c(qz.get('name',''))
        wrong_limit = int(qz.get('wrong_limit','0') or 0)
        qlimit = int(qz.get('questions','0') or 0)
        emit('  {')
        emit(f'    .id = {i},')
        emit(f'    .name = "{name}",')
        emit(f'    .wrongLimit = {wrong_limit},')
        emit(f'    .questionCount = {qlimit},')
        emit(f'    .categories = _QUIZ_CATS_{i},')
        emit(f'    .categoryCount = (u16)(sizeof(_QUIZ_CATS_{i})/sizeof(_QUIZ_CATS_{i}[0])),')
        emit('  },')
    emit('};')
    emit(f'const Quiz * const QUIZZES = QUIZZES_DATA;')
    emit(f'const u16 QUIZZES_COUNT = {len(quizzes)};')
    emit('')

        # ---- Category Question Indexes (ROM) ----
    # Build per-category lists of question indices
    cat_qidx = [[] for _ in range(len(cat_names))]
    for qi, row in enumerate(questions_rows):
        cid = cat_index[row['category'].strip()]
        cat_qidx[cid].append(qi)

    emit('// ---- Category Question Indexes ----')
    # Emit one static array per non-empty category; keep NULL for empty ones
    cat_arr_symbols = []
    for ci, lst in enumerate(cat_qidx):
        if lst:
            arrname = f'_CAT_QIDX_{ci}'
            cat_arr_symbols.append(arrname)
            emit(f'static const u16 {arrname}[] = {{ ' + ', '.join(str(x) for x in lst) + ' };')
        else:
            cat_arr_symbols.append('0')  # NULL pointer for empty category

    # Master pointer table and counts
    emit('const u16 * const CATEGORY_QUESTION_INDEXES[] = {')
    for sym in cat_arr_symbols:
        emit(f'  {sym},')
    emit('};')

    emit('const u16 CATEGORY_QUESTION_COUNTS[] = {')
    for lst in cat_qidx:
        emit(f'  {len(lst)},')
    emit('};')
    emit('')

    out_c.parent.mkdir(parents=True, exist_ok=True)
    out_c.write_text("\n".join(lines), encoding='utf-8')
    print(f"Wrote {out_c}")

if __name__ == '__main__':
    main()

#python3 thirdparty/scripts/compile_data.py /data/scenes.txt data/questions.csv data/quizzes.txt src/data_load.c
