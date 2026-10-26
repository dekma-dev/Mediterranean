"""Read a user-authored Blender collection; export fitted guardian plates only.
Run: blender -b SOURCE --python scripts/export-guardian-armor.py
Never saves or changes the source file. Exports chest, shoulders and fauld with two native OBJ LODs each.
"""
import bpy, bmesh, hashlib, json
from pathlib import Path
from mathutils import Vector

out = Path(__file__).resolve().parents[1] / 'assets' / 'characters'
collection = next(c for c in bpy.data.collections if c.name.startswith('01 '))
groups = {
 'cuirass': (('REWORK | shaped breastplate','REWORK | shaped backplate','REWORK | turned plate edge','REWORK | arm opening rolled edge'), (0,0,1.61), 1800,480),
 'shoulder-left': (('REWORK | articulated shoulder sheet','REWORK | shoulder rolled hem','REWORK | shoulder fastening'), (.40,0,1.79), 900,220),
 'shoulder-right': (('REWORK | articulated shoulder sheet','REWORK | shoulder rolled hem','REWORK | shoulder fastening'), (-.40,0,1.79), 900,220),
 'fauld': (('REWORK | articulated waist lame','REWORK | waist rolled edge','REWORK | waist sliding rivet'), (0,0,1.39), 900,220)
}
all_objects=[o for o in collection.objects if not o.hide_render and any(o.name.startswith(g[0]) for g in groups.values())]
assert all_objects, 'Authored armor not found'
root=all_objects[0].parent
objects=all_objects
# Disable armature evaluation for a stable bind-space equipment export.
for ob in objects:
    for mod in ob.modifiers:
        if mod.type == 'ARMATURE': mod.show_viewport = False
bpy.context.view_layer.update()
deps = bpy.context.evaluated_depsgraph_get()
report = {'source': str(Path(bpy.data.filepath)),
          'sha256': hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest(),
          'objects': [o.name for o in objects], 'lods': []}
jobs=[]
for group,(prefix,origin,high,low) in groups.items():
    selected=[o for o in all_objects if o.name.startswith(prefix)]
    if 'shoulder' in group:
        selected=[o for o in selected if
          (sum((root.matrix_world.inverted()@o.matrix_world@Vector(v)).x for v in o.bound_box)/8>0)==(origin[0]>0)]
    assert selected, 'Missing group '+group
    jobs.extend([(group,selected,origin,'guardian-'+group,high),
                 (group,selected,origin,'guardian-'+group+'-low',low)])
for group,objects,origin,name,budget in jobs:
    vertices, faces = [], []
    for ob in objects:
        evaluated = ob.evaluated_get(deps); mesh = evaluated.to_mesh(); mesh.calc_loop_triangles()
        transform = root.matrix_world.inverted() @ ob.matrix_world
        offset = len(vertices)
        for v in mesh.vertices:
            p = transform @ v.co
            vertices.append((p.x-origin[0], p.z-origin[2], -(p.y-origin[1])))
        # Rotate Z-up/-Y forward to engine Y-up/+Z, retaining face winding.
        faces.extend(tuple(offset+t.vertices[i] for i in (0,1,2)) for t in mesh.loop_triangles)
        evaluated.to_mesh_clear()
    data=bpy.data.meshes.new(name);data.from_pydata(vertices,[],faces);data.update()
    temporary=bpy.data.objects.new(name,data);bpy.context.scene.collection.objects.link(temporary)
    decimate=temporary.modifiers.new('Runtime plate budget','DECIMATE')
    decimate.ratio=min(1, budget/len(faces));decimate.use_collapse_triangulate=True
    bpy.context.view_layer.update();evaluated=temporary.evaluated_get(bpy.context.evaluated_depsgraph_get())
    reduced=bpy.data.meshes.new_from_object(evaluated)
    bm=bmesh.new();bm.from_mesh(reduced);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(reduced);bm.free()
    reduced.calc_loop_triangles()
    lines=['# Authored Dawnwarden cuirass; metres, Y up, +Z forward; torso-local', 's 1']
    for v in reduced.vertices:
        p=v.co
        assert max(abs(x) for x in p)<.501, 'Attachment envelope exceeded'
        lines.append('v %.9g %.9g %.9g .72 .77 .78' % tuple(p))
    normals=[v.normal.copy() for v in reduced.vertices]
    for t in reduced.loop_triangles:
        a,b,c=[reduced.vertices[i].co for i in t.vertices]
        n=(b-a).cross(c-a)
        if n.length_squared>1e-16:
            n.normalize()
            for i in t.vertices:
                if normals[i].length_squared<1e-12: normals[i]=n.copy()
    for n in normals:
        if n.length_squared<1e-12: n=Vector((0,1,0)) # Unused collapsed vertex.
        lines.append('vn %.9g %.9g %.9g' % tuple(n))
    normal_count=len(reduced.vertices)
    for t in reduced.loop_triangles:
        ids=list(t.vertices)
        a,b,c=[reduced.vertices[i].co for i in ids]
        normal=(b-a).cross(c-a)
        if normal.length_squared<1e-16: continue
        average=sum((normals[i] for i in ids),Vector())
        if average.dot(normal)<=0:
            # Thin rolled edges can average opposite surface normals after
            # collapse. Keep those corners hard instead of importing inverted light.
            normal.normalize();lines.append('vn %.9g %.9g %.9g' % tuple(normal))
            normal_count+=1
            lines.append('f '+' '.join(f'{i+1}//{normal_count}' for i in ids))
        else:
            lines.append('f '+' '.join(f'{i+1}//{i+1}' for i in ids))
    (out/(name+'.obj')).write_text('\n'.join(lines)+'\n',encoding='utf-8')
    report['lods'].append({'name': name,'vertices':len(reduced.vertices),'triangles':len(reduced.loop_triangles)})
    bpy.data.objects.remove(temporary,do_unlink=True)
(out/'guardian-cuirass-source.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report))
