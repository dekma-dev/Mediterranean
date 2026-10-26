"""Audit player-facing C++/quest text against the Russian catalog (stdlib only)."""
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOKEN = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|(?:u8|L)?R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\([\s\S]*?\)(?P=delimiter)"|(?:L|u8)?"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
PAIR = re.compile(r'\{("(?:\\.|[^"\\])*")\s*,\s*("(?:\\.|[^"\\])*")\}')
ALLOWED = {'WASD','Tab','Enter','ENTER','Esc','Escape','Ctrl','Alt','Shift','II','III','English','DirectX','Blender','PNG','BGRA','RPGNative','RPGNative.exe','MediterraneanLauncher.exe','START_AND_DEBUG.md'}

def catalog():
    result = {}
    for a,b in PAIR.findall((ROOT/'game/Russian.inc').read_text(encoding='utf-8')):
        k,v=json.loads(a),json.loads(b)
        if not k or not v: raise ValueError('Empty localization entry')
        if k in result: raise ValueError('Duplicate localization key: '+k)
        result[k]=v
    return result

def literals(path):
    source=path.read_text(encoding='utf-8'); previous=None
    for match in TOKEN.finditer(source):
        token=match.group()
        if not re.match(r'^(?:L|u8)?(?:R)?"',token): continue
        prefix=source[source.rfind('\n',0,match.start())+1:match.start()]
        if re.match(r'\s*#\s*include',prefix): continue
        if re.match(r'^(?:L|u8)?R"',token):
            delimiter=match.group('delimiter');value=token[token.index('(')+1:-(len(delimiter)+2)]
        else:
            try:value=json.loads(token[token.index('"'):])
            except ValueError: continue
        if previous and not source[previous[2]:match.start()].strip():
            previous=(previous[0]+value,previous[1],match.end())
        else:
            if previous: yield previous[0],previous[1]
            previous=(value,source.count('\n',0,match.start())+1,match.end())
    if previous: yield previous[0],previous[1]

def inventory():
    files=sorted((ROOT/'game').glob('*.h'))+sorted((ROOT/'game').glob('*.cpp'))+sorted((ROOT/'launcher').glob('*.cpp'))
    files += [ROOT/p for p in ['launcher/LaunchSupport.h','main.cpp','platform/Win32Window.cpp','engine/Engine.cpp','quests/QuestText.h.in']]
    for path in files:
        if path.name in ('Localization.h','Russian.inc'):continue
        for value,line in literals(path):
            if re.search(r'[A-Za-z]{2}',value):yield value,str(path.relative_to(ROOT)).replace('\\','/')+':'+str(line)
    # Renderer operation/error captions are shown in native message boxes;
    # its narrow strings are shader semantics and developer benchmark output.
    source=(ROOT/'engine/Renderer.cpp').read_text(encoding='utf-8')
    for m in re.finditer(r'\bL("(?:\\.|[^"\\])*")',source):
        try:value=json.loads(m[1])
        except ValueError:continue
        if re.search('[A-Za-z]{2}',value):yield value,'engine/Renderer.cpp:'+str(source.count('\n',0,m.start())+1)
    for path in sorted((ROOT/'quests').glob('*/*.txt')):
        yield path.read_text(encoding='utf-8').strip(),str(path.relative_to(ROOT)).replace('\\','/')

def translate(value,d):
    if value in d:return d[value]
    buckets={}
    for k in sorted(d,key=len,reverse=True):buckets.setdefault(k[0],[]).append(k)
    letter=lambda c:c.isascii() and c.isalpha()
    out='';i=0
    while i<len(value):
        for k in buckets.get(value[i],[]):
            end=i+len(k)
            if not value.startswith(k,i):continue
            if (i and letter(value[i-1]) and letter(k[0])) or (end<len(value) and letter(value[end]) and letter(k[-1])):continue
            out+=d[k];i=end;break
        else:out+=value[i];i+=1
    return out

def foreign(value):
    return [w for w in re.findall(r'[A-Za-z_][A-Za-z_0-9.]*',value) if len(w.rstrip('.'))>1 and w.rstrip('.') not in ALLOWED and not re.fullmatch(r'F\d+|x\d+|0x[0-9A-Fa-f]*',w.rstrip('.'))]

def main():
    d=catalog(); exclusions=json.loads((ROOT/'tests/localization-nonplayer.json').read_text(encoding='utf-8'))
    errors=[];count=0
    for value,location in inventory():
        if value in exclusions:continue
        result=translate(value,d);left=foreign(result)
        if left:errors.append({'source':location,'english':value,'russian':result,'remaining':left})
        else:count+=1
    for key,value in d.items():
        if foreign(value):errors.append({'catalog':key,'russian':value,'remaining':foreign(value)})
    if errors:
        print(json.dumps(errors,ensure_ascii=False,indent=2));return 1
    print(f'PASS: {count} source occurrences; {len(d)} Russian entries; no untranslated player prose. Technical identifiers/hotkeys are explicitly allowed.')
    return 0
if __name__=='__main__':raise SystemExit(main())
