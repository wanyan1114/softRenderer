# 鍥哄畾 4x MSAA 鏀寔

## 鍔熻兘鑳屾櫙

褰撳墠浠撳簱宸茬粡瀹屾垚浜嗗簲鐢ㄥ眰銆佸満鏅眰銆佺浉鏈哄眰銆佽祫婧愬姞杞藉眰鍜?`Vertex -> Clip -> Raster -> Fragment` 鐨勫熀纭€杞厜鏍呬富绾匡紝骞朵笖 viewer 鍙互绋冲畾鏄剧ず甯?base color 绾圭悊鐨?OBJ 妯″瀷銆?
浣嗗綋鍓?`Framebuffer` 浠嶆槸鍗曟牱鏈鑹茬紦鍐?+ 鍗曟牱鏈繁搴︾紦鍐诧紝涓夎褰㈣鐩栧垽鏂€佹繁搴︽祴璇曞拰棰滆壊鍐欏叆閮藉彂鐢熷湪鍍忕礌涓績鐐广€傝繖浼氬鑷存ā鍨嬭疆寤撳湪鏂滆竟鍜岃繙澶勭粏鑺備笂鍑虹幇鏄庢樉閿娇銆?
鏈疆鐩爣鏄湪涓嶆敼鍙樺綋鍓嶆ā鍧楄竟鐣岀殑鍓嶆彁涓嬶紝涓哄綋鍓?viewer 琛ヤ笂鍥哄畾 4x MSAA 鎶楅敮榻胯兘鍔涖€?
## 闇€姹傛憳瑕?
- 涓哄綋鍓?viewer 娓叉煋璺緞鍔犲叆鍥哄畾 4x MSAA銆?- `Framebuffer` 闇€瑕佸悓鏃剁淮鎶わ細
  - 鏈€缁堝儚绱犻鑹茬紦鍐?  - 鏈€缁堝儚绱犳繁搴︾紦鍐?  - 4x sample 棰滆壊缂撳啿
  - 4x sample 娣卞害缂撳啿
- `PipelineStages` 鐨勫厜鏍呭寲闃舵瑕佹寜 sample 鍋氳鐩栧垽鏂拰娣卞害娴嬭瘯銆?- 甯ф樉绀哄墠闇€瑕佹妸 sample 棰滆壊 resolve 鎴愭渶缁堝儚绱犻鑹层€?- `Window::Present()` 缁х画鍙秷璐?resolved 鍚庣殑鍍忕礌缂撳啿銆?- 榛樿 lit OBJ viewer 鐨勫姛鑳姐€佺浉鏈烘帶鍒躲€佺汗鐞嗛噰鏍峰拰鑳岄潰鍓旈櫎琛屼负涓嶅洖閫€銆?
鏈疆涓嶅仛锛?
- 杩愯鏃跺垏鎹?MSAA 绛夌骇
- 2x / 8x 绛夋洿澶氱瓑绾?- 閫忔槑娣峰悎
- 鍙厤缃?sample pattern UI
- JobSystem 骞惰 resolve
- 娣卞害缂撳啿鐨勯澶栧鍑烘垨 debug 鍙鍖?
## 瀵瑰簲闃呰鐨勯」鐩灦鏋勫浘

- `docs/architecture/project-architecture.md`

## 瀵瑰簲闃呰鐨?`computer-graphics-notes` 妯″潡

- `computer-graphics-notes/lesson04-framebuffer-depth-and-msaa.md`
- `computer-graphics-notes/lesson06-software-rasterization-mainline.md`

## 鏋舵瀯鍥句笌绗旇瀵瑰綋鍓嶅姛鑳界殑绾︽潫/鍚彂

- 鏋舵瀯鍥剧害鏉熻繖娆℃敼鍔ㄥ簲涓昏钀藉湪 `src/render` 鍐呴儴锛屼笉鎶?MSAA 閲囨牱閫昏緫鎵╂暎鍒?`src/app`銆乣src/resource` 鎴?`src/platform`銆?- `lesson04` 绾︽潫 `Framebuffer` 搴斾綔涓?sample 绾ч鑹?娣卞害缁撴灉涓庢渶缁堟樉绀哄儚绱犱箣闂寸殑妗ユ帴瀹瑰櫒銆?- `lesson04` 鍚彂褰撳墠鏈€鍚堢悊鐨勬暟鎹祦鏄細sample 鍐欏叆 -> resolve -> `Window::Present()`銆?- `lesson06` 绾︽潫 MSAA 鍙戠敓鍦ㄥ厜鏍呭寲鍜岀墖鍏冮樁娈碉紝涓嶅簲鏀瑰彉 `Vertex -> Clip -> Raster -> Fragment` 鐨勬€讳綋椤哄簭銆?
## 褰撳墠浠ｇ爜涓庣洰鏍囩粨鏋勫樊寮?
褰撳墠鐘舵€侊細

- `Framebuffer` 鍙湁鍗曟牱鏈?`m_Pixels` 鍜?`m_DepthBuffer`
- `RasterizeTriangle()` 鍙湪鍍忕礌涓績鍙栦竴涓?sample point
- 娣卞害娴嬭瘯鍜岄鑹插啓鍏ュ彧鍙戠敓涓€娆?- `Window::Present()` 鐩存帴鏄剧ず褰撳墠 `m_Pixels`

鐩爣鐘舵€侊細

- `Framebuffer` 鏂板鍥哄畾 4x sample 棰滆壊/娣卞害缂撳啿
- `RasterizeTriangle()` 瀵规瘡涓儚绱犵殑 4 涓?sample 鍒嗗埆鍒ゆ柇瑕嗙洊
- 娣卞害娴嬭瘯鏀逛负鎸?sample 鎵ц
- 鐗囧厓棰滆壊鎸夎鐩栧埌鐨?sample 鍐欏叆
- 甯ф湯缁熶竴 resolve 鍒板儚绱犵紦鍐插悗鍐嶆樉绀?
## 鎶€鏈柟妗堜笌鍏抽敭鏁版嵁娴?
### 1. Framebuffer 鎵╁睍

鍦?`src/render/Framebuffer.*` 涓墿灞曪細

- 淇濈暀鐜版湁 `m_Pixels`
- 淇濈暀鐜版湁 `m_DepthBuffer`
- 鏂板 `m_SamplePixels`
- 鏂板 `m_SampleDepthBuffer`
- 鏂板鍥哄畾 4x sample 鍋忕Щ瀹氫箟
- 鏂板 sample 绱㈠紩杈呭姪鍑芥暟
- 鏂板 `ResolveColor()` 鎺ュ彛

鍏朵腑锛?
- `m_Pixels` 缁х画浣滀负 present 浣跨敤鐨?resolved 棰滆壊缂撳啿
- `m_SamplePixels` 浣滀负鍏夋爡鍖栭樁娈电湡姝ｅ啓鍏ョ殑棰滆壊缂撳啿
- `m_SampleDepthBuffer` 浣滀负 sample 绾ф繁搴︽祴璇曠紦鍐?- `m_DepthBuffer` 鏈疆淇濇寔涓庡儚绱犺涔夊吋瀹癸紝涓昏鐢ㄤ簬淇濈暀鐜版湁鎺ュ彛鍜屽繀瑕佹椂鎻愪緵鎸夊儚绱犳繁搴﹁鍙?
### 2. 4x sample pattern

鏈疆鍥哄畾浣跨敤 4 涓儚绱犲唴鍋忕Щ鐐癸紝渚嬪锛?
```text
(0.375, 0.125)
(0.875, 0.375)
(0.125, 0.625)
(0.625, 0.875)
```

鐩爣鏄杈圭紭瑕嗙洊缁熻鏇寸ǔ瀹氾紝鍚屾椂淇濇寔瀹炵幇绠€鍗曘€?
### 3. 鍏夋爡鍖栭樁娈佃皟鏁?
鍦?`src/render/PipelineStages.h` 涓細

- 淇濈暀鍖呭洿鐩掗亶鍘?- 瀵规瘡涓儚绱狅紝閬嶅巻 4 涓?sample 鐐?- 浣跨敤 sample 鐐瑰仛 edge function 瑕嗙洊鍒ゆ柇
- 瀵硅鐩栧埌鐨?sample 鍒嗗埆鍋氭繁搴︽祴璇?- 鑻ユ煇涓?sample 閫氳繃娣卞害娴嬭瘯锛屽垯灏嗙墖鍏冮鑹插啓鍏ュ搴?sample 棰滆壊缂撳啿

涓轰簡鎺у埗瀹炵幇澶嶆潅搴︼紝鏈疆淇濇寔锛?
- 姣忎釜鍍忕礌鍙彃鍊间竴娆?varyings
- 鎻掑€间綅缃娇鐢ㄥ儚绱犱腑蹇?- coverage 鍜?depth 鎸?sample 璁＄畻

杩欏拰 lesson04 閲屸€渟ample 绾ц鐩?+ sample 绾ф繁搴?+ resolve鈥濅负涓荤殑鐩爣涓€鑷淬€?
### 4. Resolve 涓庢樉绀?
姣忓抚鍦?`RenderLayer::OnRender()` 瀹屾垚鎵€鏈?draw 鍚庯細

```text
sample color buffer
-> Framebuffer::ResolveColor()
-> pixel color buffer
-> Window::Present(framebuffer)
```

`Window::Present()` 涓嶉渶瑕佺煡閬?sample 缂撳啿锛屽彧缁х画璇诲彇 `Framebuffer::Data()`銆?
## 璁″垝淇敼鐨勬枃浠?
- `docs/features/fixed-4x-msaa-support.md`
- `src/render/Framebuffer.h`
- `src/render/Framebuffer.cpp`
- `src/render/PipelineStages.h`
- `src/app/layer/RenderLayer.cpp`
- `docs/architecture/project-architecture.md`

## 椋庨櫓鐐逛笌鍏煎鎬у奖鍝?
- `Framebuffer` 鏄綋鍓?render 鍜?platform 鐨勫叡浜牳蹇冩暟鎹粨鏋勶紝鏈疆鎵╁睍闇€瑕侀伩鍏嶇牬鍧?`Window::Present()` 瀵?resolved 缂撳啿鐨勮鍙栫害瀹氥€?- 鑻?sample 绱㈠紩鍜屽寘鍥寸洅閬嶅巻鐨勮绱㈠紩璁＄畻涓嶄竴鑷达紝瀹规槗鍑虹幇杈圭紭闂儊鎴栭敊璇啓鍏ャ€?- 鑻?resolve 椤哄簭涓嶅锛屽彲鑳藉嚭鐜颁笂涓€甯ч鑹叉畫鐣欐垨鏄剧ず鏈В鏋愮殑缁撴灉銆?- 鏈疆浼氭敼鍙?`Framebuffer` 鐨勬牳蹇冭涔夛紝鍥犳闇€瑕佹鏌ュ苟鍚屾鏇存柊椤圭洰鏋舵瀯鏂囨。銆?
## 鎵嬪姩娴嬭瘯鏂规

1. 缂栬瘧 / 杩愯
   - `cmake --build build-mingw --target sr_viewer`
   - `build-mingw/bin/sr_viewer.exe`
2. 鍚姩楠岃瘉
   - 榛樿 OBJ 鍦烘櫙姝ｅ父鏄剧ず
   - 鐩告満绉诲姩鍜岃浆鍚戞帶鍒朵繚鎸佸彲鐢?3. MSAA 瑙傚療鐐?   - 绔嬫柟浣撹疆寤撹竟缂樼浉姣斿崟鏍锋湰鐗堟湰鏇村钩婊?   - 杩滃鏂滆竟鍜岄珮瀵规瘮杈圭晫鐨勯敮榻挎劅鍑忓急
4. 娓叉煋姝ｇ‘鎬?   - 绾圭悊閲囨牱浠嶆甯?   - 閬尅鍏崇郴浠嶆甯?   - 鏃犳槑鏄鹃粦杈广€侀棯鐑併€侀殢鏈哄櫔鐐?5. 濡傜粨鏋滃紓甯革紝浼樺厛妫€鏌?   - sample 鍋忕Щ瀹氫箟
   - sample 娣卞害绱㈠紩
   - resolve 鏃舵満

## 瀹屾垚鏍囧噯 / 楠屾敹鏍囧噯

1. 宸插畬鎴愰渶姹傚榻?2. 宸茬敓鎴愬紑鍙戞枃妗?3. 褰撳墠 viewer 宸叉敮鎸佸浐瀹?4x MSAA
4. `Framebuffer` 宸叉敮鎸?sample 绾ч鑹?娣卞害缂撳啿涓?resolve
5. 鍏夋爡鍖栭樁娈靛凡鎸?sample 鍋氳鐩栧垽鏂拰娣卞害娴嬭瘯
6. `Window::Present()` 浠嶅彧娑堣垂 resolved 鍍忕礌缂撳啿
7. 宸叉鏌ュ苟鍦ㄩ渶瑕佹椂鏇存柊鏋舵瀯鏂囨。
8. 宸叉彁渚涙墜鍔ㄦ祴璇曟柟寮?9. 绛夊緟鐢ㄦ埛 review
