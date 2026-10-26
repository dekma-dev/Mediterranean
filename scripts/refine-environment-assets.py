"""Run with Blender -b collection01.blend --python this-file -- OUTPUT.blend.

Refines existing environment objects only. Never regenerates the collection.
Writes a separate candidate so the original can be backed up and reviewed.
"""
import bpy
import math
import sys
from pathlib import Path

output=Path(sys.argv[sys.argv.index('--')+1]).resolve()
if output==Path(bpy.data.filepath).resolve() or output.exists():
    raise RuntimeError('Use a new candidate path; do not overwrite the source')
pine=next(c for c in bpy.data.collections if c.name.startswith('09 '))
if pine.get('environment_refinement'):
    raise RuntimeError('This collection is already refined')

def spray(material):
    vertices=[]; faces=[]
    # Closed, thin diamond-shaped shoots replace each solid conical bough.
    # Their sparse silhouette exposes the existing trunk and branch structure.
    for row in range(5):
        z=-.46+row*.185
        spread=(1-row*.14)*.93
        for side in (-1,1):
            start=len(vertices)
            vertices.extend([(.04*side,-.07,z-.115),(.04*side,.07,z+.115),
                             (side*spread,.025,z-.16),(.13*side,.14,z+.105)])
            faces.extend([(start,start+2,start+1),(start,start+3,start+2),
                          (start+1,start+2,start+3),(start,start+1,start+3)])
    mesh=bpy.data.meshes.new('Highland pine layered needle shoots')
    mesh.from_pydata(vertices,[],faces);mesh.materials.append(material);mesh.update()
    return mesh

materials={}
for ob in pine.objects:
    if not ob.name.startswith(('Needle bough','Crown leader')): continue
    old=ob.data.materials[0]
    if old.name not in materials:
        mat=old.copy();mat.name='Highland needle foliage '+str(len(materials))
        color=(.075,.125,.075,1) if len(materials)==0 else (.10,.16,.085,1)
        mat.diffuse_color=color
        if mat.use_nodes:
            shader=mat.node_tree.nodes.get('Principled BSDF')
            if shader: shader.inputs['Base Color'].default_value=color;shader.inputs['Roughness'].default_value=.9
        materials[old.name]=spray(mat)
    ob.data=materials[old.name]
    # Preserve every existing attachment/transform, with small deterministic
    # variation around the branch axis to break the regular tier silhouette.
    phase=sum(ord(c) for c in ob.name)
    ob.rotation_euler.rotate_axis('Z',math.sin(phase*1.71)*.35)
pine['environment_refinement']='Layered needle shoots; original branch placement retained'

palette={'oak grain':(.105,.074,.047,1),'limewash':(.40,.37,.29,1),
         'slate':(.075,.105,.12,1),'limestone':(.27,.275,.25,1)}
copies={}
for col in bpy.data.collections:
    if not col.name.startswith(('11 ','12 ','13 ')): continue
    for ob in col.objects:
        if ob.type!='MESH' or not ob.parent: continue
        for slot in ob.material_slots:
            material=slot.material
            if not material: continue
            match=next((value for key,value in palette.items() if key in material.name.lower()),None)
            if match is None: continue
            if material.name not in copies:
                copy=material.copy();copy.name='Weathered '+material.name;copy.diffuse_color=match
                if copy.use_nodes:
                    shader=copy.node_tree.nodes.get('Principled BSDF')
                    if shader: shader.inputs['Base Color'].default_value=match;shader.inputs['Roughness'].default_value=.88
                copies[material.name]=copy
            # Object-linked slots prevent changes to shared actor/prop meshes.
            slot.link='OBJECT';slot.material=copies[material.name]
output.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(output))
print('ENVIRONMENT_CANDIDATE',output)
