case E_SNAPPER:{
 const unsigned variant=e.elite>=2?950u:e.elite?800u:700u;
 int scale=(int)((int64_t)projection*e.size_scale*660*variant/(6000*1000));
 int cx=(int)((int64_t)-22*scale/65536),cy=-(scale/65536);
 int ax=x-(cx*dirX(e.angle)-cy*dirY(e.angle))/256,ay=y-(cx*dirY(e.angle)+cy*dirX(e.angle))/256;
 source(c,"sGlowWhite",ax,ay,(int)((int64_t)projection*e.size_scale/10),e.angle,false,65535,40,true);
 source(c,"sSquidHead",ax,ay,scale,e.angle,false,e.flash?rgb(255,242,222):65535);
 return true;
}
