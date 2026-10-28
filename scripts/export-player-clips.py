from pathlib import Path
import json,struct,hashlib
import numpy as np
import argparse
parser=argparse.ArgumentParser(description='Export existing Collection 04 JSON actions without opening or saving Blender sources.')
parser.add_argument('--source',type=Path,default=Path('.blender-local/collection-02/collection-04/animation'))
parser.add_argument('--output',type=Path,default=Path('assets/authored'))
args=parser.parse_args();source=args.source;out=args.output;out.mkdir(parents=True,exist_ok=True)
clips=['Attack_Slash','Attack_Overhead','Jump_Full','Land_Heavy','Walk_Forward','Walk_Backward','Strafe_Left','Strafe_Right','Run_Forward','Run_Backward','Run_Left','Run_Right','Start_Walk','Stop_Walk']
sk=json.loads((source/'01-skeleton.json').read_text());bones=sk['bones'];rest={b['name']:np.array(b['rest_matrix']) for b in bones};parents={b['name']:b['parent'] for b in bones}
def rotation(q):
 w,x,y,z=q
 return np.array([[1-2*y*y-2*z*z,2*x*y-2*z*w,2*x*z+2*y*w],[2*x*y+2*z*w,1-2*x*x-2*z*z,2*y*z-2*x*w],[2*x*z-2*y*w,2*y*z+2*x*w,1-2*x*x-2*y*y]])
C=np.array([[1,0,0],[0,0,1],[0,-1,0]])
archive=bytearray(struct.pack('<4sII',b'PCL1',1,len(clips)));report=[]
for clip in clips:
 f=source/f'01-{clip}.json';d=json.loads(f.read_text());frames=[]
 assert d['fps']==30 and not d['root_motion']
 assert abs(d['duration']-(len(d['samples'])-1)/30)<1e-6
 for index,row in enumerate(d['samples']):
  transforms={}
  for name,values in zip(d['bones'],row):
   local=np.eye(4);local[:3,:3]=rotation(values[3:]);local[:3,3]=values[:3];par=parents[name];localrest=np.linalg.inv(rest[par])@rest[name] if par else rest[name];transforms[name]=(transforms[par] if par else np.eye(4))@localrest@local
  pelvis=transforms['pelvis'][:3,3];R=transforms['chest'][:3,:3]@rest['chest'][:3,:3].T
  t=index/(len(d['samples'])-1);air=.62*4*((t-.23)/.53)*(1-(t-.23)/.53) if clip=='Jump_Full' and .23<t<.76 else 0
  values=[]
  # Blender L is +X; the player's right-hand weapon is also +X.
  for side in ['R','L']:
   name='hand.'+side;local=R.T@(transforms[name][:3,3]-pelvis)+rest['pelvis'][:3,3]
   values.extend(C@(local-rest[name][:3,3])*.78)
  for side in ['R','L']:
   name='foot.'+side;delta=transforms[name][:3,3]-rest[name][:3,3];delta[2]-=air;values.extend(C@delta*.82)
  values.append((pelvis[2]-air-rest['pelvis'][2,3])*.82)
  hand=transforms['hand.L'][:3,:3]@rest['hand.L'][:3,:3].T
  weapon=C@(R.T@hand@np.array([0,0,1.]));weapon/=np.linalg.norm(weapon);values.extend(weapon)
  gameR=C@R@C.T;values.extend([np.arctan2(-gameR[1,2],gameR[1,1]),np.arctan2(-gameR[2,0],gameR[0,0]),np.arcsin(np.clip(gameR[1,0],-1,1))])
  assert len(values)==19 and np.isfinite(values).all()
  frames.append(struct.pack('<19f',*values))
 archive+=struct.pack('<If',len(frames),d['duration'])+b''.join(frames)
 report.append(dict(clip=clip,frames=len(frames),duration=d['duration'],source_sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
assert len(archive)<=128*1024
(out/'player.clips').write_bytes(archive)
(out/'player-clips.json').write_text(json.dumps(dict(source='Collection 04 Blender actions / Dawnwarden',skeleton_sha256=hashlib.sha256((source/'01-skeleton.json').read_bytes()).hexdigest(),clips=report,sha256=hashlib.sha256(archive).hexdigest(),retarget='Rest-relative hand and ankle targets, pelvis offset, weapon direction. Blender Z-up converted to game Y-up. Jump world displacement removed; gameplay owns vertical movement.'),indent=2),encoding='utf8')
print('Exported',len(archive),'bytes',out/'player.clips')
