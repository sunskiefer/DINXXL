import cv2, numpy as np, os
SRC=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','art','src')
rng=np.random.default_rng(1)
GREEN="#1ab463"
def clean(name, zones, boxes, vlines=True):
    im=cv2.imread(os.path.join(SRC,name+'.png'))
    g=cv2.cvtColor(im,cv2.COLOR_BGR2GRAY)
    m=(g>150).astype(np.uint8)
    keep=np.zeros_like(m)
    if vlines:
        v=cv2.morphologyEx(m,cv2.MORPH_OPEN,np.ones((60,1),np.uint8))
        hh=cv2.morphologyEx(m,cv2.MORPH_OPEN,np.ones((1,60),np.uint8))
        keep=cv2.dilate(v|hh,np.ones((5,5),np.uint8))
    sel=np.zeros_like(m)
    for (x0,y0,x1,y1) in zones+boxes: sel[y0:y1,x0:x1]=1
    t=(m&sel)&(1-keep)
    t=cv2.dilate(t,np.ones((3,3),np.uint8),iterations=4)
    t=t&(1-keep)
    f=im.astype(np.float32)
    L=g.astype(np.float32)
    bgmed=np.median(L[(L>38)&(L<70)])
    valid=((np.abs(L-bgmed)<9)&(t==0)&(cv2.dilate(m,np.ones((9,9),np.uint8))==0)).astype(np.float32)
    num=cv2.GaussianBlur(f*valid[...,None],(0,0),10); den=cv2.GaussianBlur(valid,(0,0),10)[...,None]
    base=num/np.maximum(den,1e-3)
    # grain tile from a clean margin patch of this page
    px,py=(450,335) if name=='keys' else (6,150)
    tile=f[py:py+32,px:px+64] if name=='keys' else f[py:py+96,px:px+64]
    tile=tile-cv2.GaussianBlur(tile,(0,0),5)
    grain=np.tile(tile,(im.shape[0]//tile.shape[0]+1,im.shape[1]//tile.shape[1]+1,1))[:im.shape[0],:im.shape[1]]
    a=cv2.GaussianBlur(t.astype(np.float32),(0,0),1.3)[...,None]
    a=np.clip(a*1.6,0,1)
    out=f*(1-a)+(base+grain)*a
    out=np.clip(out,0,255).astype(np.uint8)
    return finish(out,name,base,grain)

def finish(out,name,base,grain):
    # 1) remove specks / stray marks in the page margins, 2) drop the stray bottom row, 3) match page tone
    g=cv2.cvtColor(out,cv2.COLOR_BGR2GRAY)
    H,W=g.shape
    z=np.zeros((H,W),np.uint8)
    if name=='keys':
        z[:3,:]=1
    else:
        z[:,:76]=1; z[:,1268:]=1; z[622:,:]=1
    sp=((g>100)|(g<24)).astype(np.uint8)&z
    if name!='keys': sp[:, :0]=0
    sp=cv2.dilate(sp,np.ones((3,3),np.uint8),iterations=3)
    a=np.clip(cv2.GaussianBlur(sp.astype(np.float32),(0,0),1.2)[...,None]*1.6,0,1)
    o=out.astype(np.float32)
    o=o*(1-a)+(base+grain)*a
    # tone match: gain so each page's plain background sits at the same level
    gg=cv2.cvtColor(np.clip(o,0,255).astype(np.uint8),cv2.COLOR_BGR2GRAY).astype(np.float32)
    bg=np.median(gg[(gg>38)&(gg<70)])
    o*= (50.0/bg)
    return np.clip(o,0,255).astype(np.uint8)


def lamps_v3(im):
    """Recolour only the lamp discs (same lamp shape as the red REC lamp); off lamps = darker green; keys untouched."""
    orig=cv2.imread(os.path.join(SRC,'keys.png'))
    hs=cv2.cvtColor(orig,cv2.COLOR_BGR2HSV)
    H,S,V=[hs[...,i].astype(int) for i in range(3)]
    strict=(((H<=22)|(H>=170))&(S>110)&(V>90)).astype(np.uint8)
    strict[165:285,240:330]=0
    n,lab,st,cen=cv2.connectedComponentsWithStats(strict,8)
    out=im.copy(); cents=[]; Rw=24
    for i in range(1,n):
        x,y,w,h,a=st[i]
        if a<40 or a>2500 or w>40 or h>40: continue
        cx,cy=cen[i]; mx,my=int(round(cx)),int(round(cy))
        core_m=np.hypot(np.arange(orig.shape[1])[None,:]-cx,np.arange(orig.shape[0])[:,None]-cy)<4
        lit=8<=H[core_m].mean()<=30
        y0,y1,x0,x1=my-Rw,my+Rw,mx-Rw,mx+Rw
        crop=out[y0:y1,x0:x1]
        yy,xx=np.mgrid[y0:y1,x0:x1]; r=np.hypot(xx-cx,yy-cy)
        hv=cv2.cvtColor(crop,cv2.COLOR_BGR2HSV).astype(np.float32)
        ring=(r>=13.5)&(r<18)&(((hv[...,0]<=25)&(hv[...,1]>90))|((hv[...,0]>=150)&(hv[...,1]>16)))
        hv2=hv.copy(); hv2[...,0][ring]=108; hv2[...,1][ring]*=0.45
        al=np.clip((14.5-r)/2.0,0,1)
        disc=hv2.copy(); disc[...,0]=73
        if not lit:  # off = a darker green; glossy highlight stays white so it keeps the lamp's glass look
            wgt=np.clip((disc[...,1]-60)/140.0,0,1)
            disc[...,2]*=(1-0.12*wgt)
        newd=cv2.cvtColor(disc.astype(np.uint8),cv2.COLOR_HSV2BGR).astype(np.float32)
        base=cv2.cvtColor(hv2.astype(np.uint8),cv2.COLOR_HSV2BGR).astype(np.float32)
        # only touch pixels inside the ring/disc; everything else keeps its exact original value
        touch=(r<16)
        mixed=base*(1-al[...,None])+newd*al[...,None]
        res=crop.astype(np.float32)
        res[touch]=mixed[touch]
        out[y0:y1,x0:x1]=np.clip(res,0,255).astype(np.uint8)
        cents.append((mx,my,bool(lit)))
    return out,cents
