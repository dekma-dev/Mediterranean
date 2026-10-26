import bpy, bmesh, os, math, struct, json, hashlib
from mathutils import Vector
ROOT=os.path.abspath(os.path.join(os.path.dirname(__file__),'..','assets','authored'))
OUT=os.path.abspath(os.path.join(ROOT,'..','..','assets','authored'))
os.makedirs(OUT,exist_ok=True)
manifest=json.load(open(os.path.join(ROOT,'collection01.sources.json'),encoding='utf-8'))
# Keep source collection order. Every asset has three immutable game meshes.
budgets=[9000,9000,10000,9000,6500,7500,7500,12000,4000,5000,11000,13000,9000]
def style(mat):
    n=mat.name.lower()
    if any(s in n for s in ['steel','brass','iron']):kind=7
    elif any(s in n for s in ['skin','flesh']):kind=9
    elif 'leather' in n:kind=11
    elif any(s in n for s in ['cloth','wool','silk']):kind=6
    elif any(s in n for s in ['oak grain','bark']):kind=3
    elif 'limestone' in n:kind=2
    elif 'limewash' in n:kind=4
    elif 'slate' in n:kind=5
    elif 'foliage' in n:kind=8
    elif 'luminous' in n:kind=10
    elif any(s in n for s in ['hair','coat','bristle']):kind=12
    else:kind=0
    rgb=tuple(12.92*c if c<=.0031308 else 1.055*c**(1/2.4)-.055 for c in mat.diffuse_color[:3])
    return (*rgb,float(kind))

records=[];stats=[]
for index,a in enumerate(manifest):
    print('EXPORT',a['name'],flush=True)
    root=bpy.data.objects[a['root']];col=bpy.data.collections[a['collection']]
    col.hide_viewport=False;col.hide_render=False
    bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get()
    vertices=[];faces=[];smooth=[];materials=[];styles=[];sprays=[];spray_index=0
    for ob in list(col.objects):
        if ob.parent!=root or ob.hide_render or ob.type not in ['MESH','CURVE']:continue
        if index in [10,11] and ob.name.startswith(('Barrel','Entry stair')):continue
        # Hair strands are represented by the baked coat, not thousands of tubes.
        if ob.name.startswith('Fine attached directional fur'):continue
        spray=-1
        if index==8 and ob.name.startswith(('Needle bough','Crown leader')):
            spray=spray_index;spray_index+=1
        ev=ob.evaluated_get(deps);me=ev.to_mesh();me.calc_loop_triangles()
        transform=root.matrix_world.inverted()@ob.matrix_world;offset=len(vertices)
        for vertex in me.vertices:
            p=transform@vertex.co
            if index in [10,11]:
                # Keep wall faces exact; fit ornaments inside the existing
                # camera overhang without changing collision/navigation data.
                for axis,wall in [(0,1.1 if index==10 else 1.4),(1,.95)]:
                    if abs(p[axis])>wall:p[axis]=math.copysign(wall+(abs(p[axis])-wall)*.25,p[axis])
            vertices.append(tuple(p))
        local=[]
        for material_index,mat in enumerate(me.materials):
            if material_index<len(ob.material_slots) and ob.material_slots[material_index].material:
                mat=ob.material_slots[material_index].material
            st=style(mat)
            if st not in styles:styles.append(st)
            local.append(styles.index(st))
        for tri in me.loop_triangles:
            faces.append(tuple(offset+i for i in tri.vertices));smooth.append(me.polygons[tri.polygon_index].use_smooth);sprays.append(spray)
            materials.append(local[tri.material_index] if local else 0)
        ev.to_mesh_clear()
    me=bpy.data.meshes.new('Runtime working mesh');me.from_pydata(vertices,[],faces);me.update()
    # Material slots keep decimation from crossing visibly different surfaces.
    for k in range(len(styles)):me.materials.append(bpy.data.materials.new('Export boundary '+str(k)))
    for p,sm,mi in zip(me.polygons,smooth,materials):p.use_smooth=sm;p.material_index=mi
    # Blender's two-sided viewport hid inward faces on some modeled boxes.
    # Native back-face culling requires consistent outward closed surfaces.
    bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
    ob=bpy.data.objects.new('Runtime working mesh',me);bpy.context.scene.collection.objects.link(ob)
    for lod,ratio in enumerate([1,.40,.14]):
        if index==8:
            # Tiny closed shoots cannot collapse below a tetrahedron. Thin whole
            # shoots deterministically before decimation at each distance.
            retained=[i for i,group in enumerate(sprays) if group<0 or group%[2,5,13][lod]==0]
            me.clear_geometry();me.from_pydata(vertices,[],[faces[i] for i in retained]);me.update()
            for p,i in zip(me.polygons,retained):p.use_smooth=smooth[i];p.material_index=materials[i]
            bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free()
        target=int(budgets[index]*ratio*(.52 if index==7 else 1))
        mod=ob.modifiers.new('Bounded game detail','DECIMATE');mod.ratio=min(1,target/max(1,len(faces)));mod.use_collapse_triangulate=True
        bpy.context.view_layer.update();ev=ob.evaluated_get(bpy.context.evaluated_depsgraph_get());reduced=ev.to_mesh();reduced.calc_loop_triangles()
        packed=[];indices=[];dedup={};lo=[1e9]*3;hi=[-1e9]*3
        for tri in reduced.loop_triangles:
            p=[reduced.vertices[i].co for i in tri.vertices]
            cross=(p[1]-p[0]).cross(p[2]-p[0])
            if cross.length_squared<1e-14:continue
            fn=cross.normalized();st=styles[tri.material_index]
            for vi,li in zip(tri.vertices,tri.loops):
                pos=reduced.vertices[vi].co.copy();normal=reduced.corner_normals[li].vector.copy()
                if normal.dot(fn)<=.01:normal=fn
                # Blender Z-up, -Y forward -> game Y-up, +Z forward.
                xyz=(pos.x,pos.z,-pos.y);nn=(normal.x,normal.z,-normal.y)
                key=tuple(round(v,6) for v in (*xyz,*nn,*st))
                if key not in dedup:
                    dedup[key]=len(packed);packed.append((*xyz,*nn,*st))
                    for k in range(3):lo[k]=min(lo[k],xyz[k]);hi[k]=max(hi[k],xyz[k])
                indices.append(dedup[key])
            if int(st[3])==8:
                # Thin leaves need an explicit back side for the native opaque
                # culling/shadow path; retain the same material and opposite normal.
                for vi,li in reversed(list(zip(tri.vertices,tri.loops))):
                    pos=reduced.vertices[vi].co;normal=-reduced.corner_normals[li].vector
                    if normal.dot(-fn)<=.01:normal=-fn
                    xyz=(pos.x,pos.z,-pos.y);nn=(normal.x,normal.z,-normal.y)
                    key=tuple(round(v,6) for v in (*xyz,*nn,*st))
                    if key not in dedup:dedup[key]=len(packed);packed.append((*xyz,*nn,*st))
                    indices.append(dedup[key])
        ev.to_mesh_clear();ob.modifiers.remove(mod)
        record=struct.pack('<II6f',len(packed),len(indices),*lo,*hi)
        record+=b''.join(struct.pack('<10f',*v) for v in packed)
        record+=struct.pack('<%dI'%len(indices),*indices)
        records.append(record);stats.append({'asset':a['name'],'lod':lod,'vertices':len(packed),'triangles':len(indices)//3,'bounds':[lo,hi]})
        print('LOD',lod,len(packed),len(indices)//3,flush=True)
    bpy.data.objects.remove(ob,do_unlink=True);bpy.data.meshes.remove(me)
archive=struct.pack('<4sIII',b'MDA1',1,len(manifest),3)+b''.join(records)
with open(os.path.join(OUT,'collection01.masset'),'wb') as f:f.write(archive)
with open(os.path.join(OUT,'collection01.json'),'w',encoding='utf-8') as f:json.dump({'version':1,'coordinate_system':'Y up, +Z forward, metres','source':'Mediterranean_Collection_01.blend','sha256':hashlib.sha256(archive).hexdigest(),'bytes':len(archive),'meshes':stats,'limitations':'Static exports. Procedural Blender materials mapped to native material layers and vertex colors. No Blender light baking or texture dependencies.'},f,indent=2)
print('EXPORT_COMPLETE',len(archive),flush=True)
