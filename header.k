import`csv

maxs:{[L]{max(x, y)}\L}
mins:{[L]{min(x, y)}\L}
sums:{[L]{(x + y)}\L}

avgsimpl:{[L;i] curr:L[i]%(i+1); $[i<(#L)-1;curr, avgsimpl[L;i+1];curr]}
avgs:{[L] avgsimpl[sums[L];0]}

maxswimp:{[L;w;i] curr:max(L@(((i-w)+!w)|0)); $[i<#L;curr, maxswimp[L; w; i + 1];curr]}
maxsw:{[w;L]maxswimp[L; w; 1]}

minswimp:{[L;w;i] curr:min(L@(((i-w)+!w)|0)); $[i<#L;curr, maxswimp[L; w; i + 1];curr]}
minsw:{[w;L]minswimp[L;w;1]}

avgswimp:{[L;w;s;i] s:(s+L[i])-L[i-w];curr:s%((i+1)&w);$[i<(#L)-1; curr, avgswimp[L; w; s; i+1]; curr]}
avgsw:{[w;L] avgswimp[L;w;0;0]}

sumswimp:{[L;w;s;i] s:(s+L[i])-L[i-w];$[i<(#L)-1; s, sumswimp[L; w; s; i+1]; s]}
sumsw:{[w;L] sumswimp[L;w;0;0]}

groupbyi:{[L;GV;i] 
            k:(,(L[i]));gvk:GV[k][0];
            found:$[(gvk[0]+gvk[1])>0;1;L[i] in !GV];
            cg:(,L[i])!$[found;,gvk[0],i;,(,i)]; 
            $[i<(#L)-1; groupbyi[L;(GV,cg);i+1]; (GV,cg)]}
groupbys:{[L;ll] GV1:(,(L[0]))!,(,0);$[ll>1;groupbyi[L;GV1;1];GV1]}
groupby:{[l;L] $[(#l)=0;,();groupbys[L;#l]]}
