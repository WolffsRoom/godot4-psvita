from pathlib import Path
import hashlib,struct,xml.etree.ElementTree as ET,io,json
from PIL import Image
root=Path('_build/trophy-tests/project/vita_trophies')
p=root/'vpk_inject/sce_sys/trophy/TEST40001_00/TROPHY.TRP'
b=p.read_bytes(); blank=bytearray(b); blank[28:48]=bytes(20)
assert hashlib.sha1(blank).digest()==b[28:48]
assert b[:4]==bytes.fromhex('dca24d00')
assert struct.unpack_from('>Q',b,8)[0]==len(b)
n,entrysize=struct.unpack_from('>II',b,16); assert n==5 and entrysize==64
entries={}
for i in range(n):
 e=b[64+i*64:128+i*64]; name=e[:32].split(b'\0')[0].decode();offset,size=struct.unpack_from('>QQ',e,32)
 assert offset%16==0 and offset>=64+n*64 and offset+size<=len(b)
 entries[name]=b[offset:offset+size]
 assert entries[name]==(root/'source'/name).read_bytes()
 if name.endswith('.PNG'):
  assert entries[name][24:26]==bytes([8,3]); im=Image.open(io.BytesIO(entries[name])); im.load()
  assert im.size==((320,176) if name=='ICON0.PNG' else (240,240))
conf=ET.fromstring(entries['TROPCONF.SFM']);text=ET.fromstring(entries['TROP.SFM'])
assert conf.findtext('npcommid')=='TEST40001_00'
assert text.findtext('title-name')=='Teste & Troféus'
assert text.findall('trophy')[1].get('hidden')=='yes'
assert conf.findall('trophy')[1].get('ttype')=='G'
assert json.loads((root/'achievement_to_trophy.json').read_text())=={'ACH_FIRST':0,'ACH_SECRET':1}
print('PASS: independently decoded TRP header, SHA1, table, alignment, XML escaping, PNG8 and ID mapping')
