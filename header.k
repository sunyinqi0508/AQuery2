import`csv

md:{y-x*_y%x}

maxs:{[L]{max(x, y)}\L}
mins:{[L]{min(x, y)}\L}
sums:{[L]{(x + y)}\L}

avgsimpl:{[L;i] curr:L[i]%(i+1); $[i<(#L)-1;curr, avgsimpl[L;i+1];curr]}
avgs:{[L] avgsimpl[sums[L];0]}

/ maxswimp:{[L;w;i] curr:max(L@(((i-w)+!w)|0)); $[i<#L;curr, maxswimp[L; w; i + 1];curr]}
/ maxsw:{[w;L]maxswimp[L; w; 1]}

/ minswimp:{[L;w;i] curr:min(L@(((i-w)+!w)|0)); $[i<#L;curr, maxswimp[L; w; i + 1];curr]}
/ minsw:{[w;L]minswimp[L;w;1]}

/ avgswimp:{[L;w;s;i] s:(s+L[i])-L[i-w];curr:s%((i+1)&w);$[i<(#L)-1; curr, avgswimp[L; w; s; i+1]; curr]}
/ avgsw:{[w;L] avgswimp[L;w;0;0]}

/ sumswimp:{[L;w;s;i] s:(s+L[i])-L[i-w];$[i<(#L)-1; s, sumswimp[L; w; s; i+1]; s]}
/ sumsw:{[w;L] sumswimp[L;w;0;0]}


groupby0:{[L] 
            {[x;y]
                x:$[(@x)=`i;(,(L[0]))!,(,0);x];
                k:(,(L[y]));gvk:x[k][0];
                found:$[(gvk[0]+gvk[1])>0;1;L[y] in !x];
                cg:(,L[y])!$[found;,gvk[0],y;,(,y)];
                (x,cg)}/!(#L)}

groupBy:{[x]groupBySingle:{[a;x]
        findAll:{[c;xx]
            f:{[i;c]$[(c[0])[i]~c[1];i+1;0]};
            @[!#xx;!#xx;f;(#xx)#,(xx;c)]};
        z:findAll[a;x];
        b:(findAll[0;z]_(!(1+#z)))-1;(a;b)};
    x:+x;y:?x;
    @[y;!#y;groupBySingle;(#y)#,x]}

groupby:{[L]
        L:^+L;
        dimy:(#(L[0]))-1;
        ((({[L;dim;x;y] 
            x:$[x~0;(,(dim#(L[0])),0);x];
            curr:dim#(L[y]);
            $[(dim#*x)~curr;x;((,curr,y),x)]}[L;dimy])/!(#L));(+L)[dimy]) }

lststr:{[L](+({[x;y] ($x,$y)}/L))[0]}
delist:{[L] $[(@L)in(`LL`LC`LG`L);delist[(,/L)];L]}
cntlist:{[L;i] $[(@L)in(`LL`LC`LG`L);cntlist[(,/L);i+1];i+1]}
flatRO:{[x]x[0],/:x[1]}
flatLO:{[x]x[0],\:x[1]}
flatBOTH:{[x],/(x[0],/:\:x[1])}


sumswkrl:{[L;w;x;y] ((x-L[y-w])+L[y])}
sumsw:{[w;L] $[(#L)=0;L;(sumswkrl[L;w])\@[!#L;0;L[file 0]]]}
avgswkrl:{[L;w;x;y] (x-(L[y-w]-L[y])%w)}
avgsw:{[w;L] $[(#L)=0;L;(avgswkrl[L;w])\@[!#L;0;L[0]]]}

/ minsw:{[w;L] ({[L;w;x] min(L[$[x>w;(!w) + ((x-w)+1);!(x+1)]])}[L;w])'!#L}
import`mmw
minsw:{[w;L] ret:L; mmw[ret;((`g ($@ret)[0]), (#ret), w, 65536)];ret}
maxsw:{[w;L] ret:L; mmw[ret;((`g ($@ret)[0]), (#ret), w, 65537)];ret}
minswip:{[w;L] mmw[L;((`g ($@L)[0]), (#L), w, 65536)];}
maxswip:{[w;L] mmw[L;((`g ($@L)[0]), (#L), w, 65537)];}