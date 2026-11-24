<div align="center">
  <img src="docs/images/hero-7disc.jpg" alt="7Disc Image Studio — Premium Disc Authoring & Etching Suite" width="100%">
</div>

# 7Disc Image Studio 💿

### اسطوانتك بقت لوحة — مش ملصق، **حُفَر**.

**استوديو فني كامل بيستخدم ليزر الحارق كقلم رصاص، وبيحوّل صورة عادية لرسمة محفورة على ظهر الأسطوانة —
بدون طابعة، بدون ملصقات، وبدون فلوس LightScribe.**

[![Latest release](https://img.shields.io/github/v/release/abd77o/DiscImageStudio?label=release&color=2ea44f)](https://github.com/abd77o/DiscImageStudio/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/abd77o/DiscImageStudio/total?label=downloads&color=0969da)](https://github.com/abd77o/DiscImageStudio/releases)
[![CI](https://github.com/abd77o/DiscImageStudio/actions/workflows/ci.yml/badge.svg)](https://github.com/abd77o/DiscImageStudio/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%207%2F8%2F10%2F11%20(x86%2Fx64)-0078D4.svg)](#download)
[![No runtime deps](https://img.shields.io/badge/dependencies-zero-3fb950.svg)](#features)

<br>

<a href="https://github.com/abd77o/DiscImageStudio/releases/latest/download/DiscImageStudio-win32.exe">
  <img src="https://img.shields.io/badge/%E2%AC%87%EF%B8%8F%20%D8%AD%D9%85%D9%91%D9%84%20%D8%A7%D9%84%D8%A2%D9%86%20%D9%84%D9%90%20Windows-DiscImageStudio.exe-2ea44f?style=for-the-badge" alt="حمّل الآن لـ Windows" height="46">
</a>
&nbsp;
<a href="https://github.com/abd77o/DiscImageStudio/releases">
  <img src="https://img.shields.io/badge/%D9%83%D9%84%20%D8%A7%D9%84%D8%A5%D8%B5%D8%AF%D8%A7%D8%B1%D8%A7%D8%AA-Releases-444?style=for-the-badge" alt="كل الإصدارات" height="46">
</a>

**ملف `.exe` واحد بحجم ~600 كيلوبايت · مفيش تثبيت · مفيش مكتبات ناقصة · مفيش نت مطلوب بعد التحميل**

<br>

| ١ | ٢ | ٣ |
|:--:|:--:|:--:|
| ⬇️ **حمّل** الـ `exe` | 🖱️ **شغّله كمسؤول** (Run as administrator) | 🖼️ **اختار صورتك** واحرق |

</div>

> ### ⚠️ قبل ما تدوس حرق
> الأداة دي **تجريبية**، وبتحرق الأسطوانة على مستوى الأوامر الأساسية للدرايف — يعني **الأسطوانة بتتلف** وبتتكتب مرة واحدة.
> ابدأ بـ **CD-RW**، سيب **Test write** مفعّل، واقرأ [docs/HARDWARE_NOTES.md](docs/HARDWARE_NOTES.md).
> **Experimental software that permanently destroys the disc it writes to. Start with a CD-RW.**

---

## 🎯 ليه 7Disc؟ / Why 7Disc

| | |
|---|---|
| 🔥 **مش حارق أسطوانات… ده استوديو نقش** | برامج الحرق بتكتب ملفات. إحنا بنكتب **صورة**. كل بكسل في الشكل اللي بتشوفه هو قرار حُفَر اتخد على مستوى القطاع. |
| 🪶 **من أخف استوديوهات الأقراص المفتوحة المصدر** | **صفر** اعتماديات وقت التشغيل. ملف `.exe` واحد static. مفيش .NET، مفيش redistributable، مفيش تثبيت. فك الضغط وشغّل. |
| 💾 **مبني لأجهزة الناس الحقيقية** | مُهندس عشان يشتغل على **Windows 7** وعلى جهاز بـ 512 ميجا رام. حاسبة الخطأ في الديثرينج كلها **أقل من 1 ميجا**، والرستر الداخلي **≤ 1 ميجا** — مهما كان القرص. |
| 🔬 **بيتكلم مع الحارق بلغته الأم** | أوامر MMC على مستوى SCSI: `SEND CUE SHEET` و `WRITE(10)` في وضع Disc-At-Once، مع دعم منع buffer-underrun. مفيش طبقة وسيطة تخبّي عنك حاجة. |
| 🛡️ **فلسفة: متحرقش أسطوانتك** | حارس بَفر بيقرأ امتلاء ذاكرة الدرايف لحظيًا، يوسّع الدفعات، ويقترح سرعة أهدى للمرة الجاية. والحرق الحقيقي **مقفول** لحد ما *Test write* ينجح. |
| 🎛️ **معايرة على درايفك انت** | سلّم من ١٦ نمط بايت بيتحرق كحلقات، انت تختار الأفتح والأغمق لعينك — فيتحفظ ويشتغل على ميدياك انت، مش على نظرية. |
| 🧪 **مفيش كلام بدون دليل** | **٣ مجموعات اختبار** بتشغّل مسار الحرق كامل على درايف وهمي — بدون هاردوير. وبتشتغل على Linux في كل push. |

> **الأرقام اللي تتقال:** `~610 كيلوبايت` — أقل من صورة موبايل — وده حجم **البرنامج كامل** بواجهته.
> وفي الشجرة الحالية: **٤٤ ملف C++** (من أصل ٧٦ ملف في الريبو)، **٣ مجموعات اختبار**، **صفر** اعتماديات.
>
> **بصراحة كاملة:** المشروع `v0.1.x` يعني **بداية قوية مش نهاية**. مسار الحرق متحقَّق منه آليًا على درايف وهمي،
> والنتائج الحقيقية موثّقة بصور من المؤلف. مصفوفة (درايف × ميديا × سرعة) لسه بتتبني — ودي أهم حاجة تقدر تساهم بيها.

---

## 💿 نتائج حقيقية / Real results

<div align="center">
  <img src="docs/images/real-burn-result.jpg" alt="CD-R محروق عليه رسمة أنمي" width="70%">
  <br>
  <sub><b>أسطوانة CD-R محروقة فعليًا — بكاميرا، مش برنامج تصميم.</b><br>
  الصورة كلها ناتجة من نمط الحُفَر على الجهة البيانات: مفيش ملصق، مفيش طباعة، مفيش حبر. (صورة من المؤلف <a href="https://github.com/abd77o">@abd77o</a>)</sub>
</div>

---

## 🖥️ الواجهة / Screenshots

<div align="center">
  <img src="docs/images/ui-concept.jpg" alt="تصميم مقترح لواجهة 7Disc Image Studio" width="94%">
  <br>
  <sub><b>التصميم المقترح للواجهة الحديثة</b> — Start / Design / Preview / Etch / Manage.<br>
  الواجهة المنفّذة في <code>v0.1.x</code> أبسط: نافذة ImGui واحدة فيها تبويب الصورة، تبويب المعايرة، وشريط تقدم.</sub>
</div>

---

<a id="features"></a>

## 🛠️ الميزات / Features

- **محرك ديثرينج قطبي (Polar Dithering):** الصورة بتُقرأ على طول المسار الحلزوني الحقيقي (`r² = r₀² + p·s/π`)،
  وتوزيع الخطأ بيحصل **في هندسة القرص**: للخلية اللي بعدها على المسار، وللخلية اللي على نفس الزاوية بعد دورة أو أكتر.
  كيرنليز **Floyd–Steinberg** و **Jarvis–Judice–Ninke**، من **٢ لـ ٨ درجات**.
- **ذاكرة ثابتة:** الصورة تُقرأ سطر بسطر وتُختزل لرستر مربّع صغير، والخطأ بيتخزن في ring buffer بمساحة ~دورتين —
  يعني حرق قرص ٨٠ دقيقة مش محتاج رام زيادة عن قرص ٥ دقايق.
- **تحكم كامل في القطاعات:** كل قطاع **2352 بايت** بيتكوّن بإيد الأداة (تخطيط Audio DAO افتراضيًا، ومعاه
  scrambler بتاع **ECMA-130** لتخطيط Mode-1)، فاللي بيوصل لمشفّر CIRC/EFM هو اللي اختاره الديثرينج بالظبط.
- **حارق MMC كامل:** `SET CD SPEED` → `MODE SELECT` → `SEND CUE SHEET` → `WRITE(10)` من LBA −150 →
  `SYNCHRONIZE CACHE`، عبر SPTI مع تفعيل منع buffer-underrun.
- **حارس البَفر:** بيقرأ `READ BUFFER CAPACITY` أثناء الحرق، يكبّر الدفعات لو البَفر نزل، ويقترح سرعة أهدى للمرة الجاية
  بدل ما يوقف الحرق في نصه.
- **معايرة ١٦ خطوة:** سلّم من بايتات مرشّحة بيتحرق كحلقات، تختار منه الأفتح والأغمق لدرايفك وميدياك، والقيمة تتحفظ في `config.ini`.
- **حواجز أمان:** الحرق الحقيقي مقفول لحد ما *Test write* ينجح مرة، والإلغاء بينبّه إن DAO مش قابل للاستكمال.
- **اختبارات بدون هاردوير:** المحرك، تركيب القطاعات، كل أوامر MMC، حارس البَفر، وكل تنظيم الحرق بيتستوا ضد درايف وهمي.

---

## ⚙️ كيف يعمل / How it works

<div align="center">
  <img src="docs/images/architecture.svg" alt="مسار البيانات من الصورة إلى القرص" width="100%">
</div>

1. **صورة ← رستر رمادي.** `ImageSource` بيقرأ الملف سطر بسطر (PNG/JPEG عبر `stb_image`، و BMP أصلي)
   ويختزله لرستر مربّع 8-bit بحجم ≤ 1024 × 1024.
2. **رستر ← درجات.** `PolarDitherEngine` بيمشي على الحلزون خلية خلية (196 بايت للخلية، 12 خلية في القطاع،
   ≈1.33 مم طول مسار). كل خلية بتُقرأ عند (نصف قطر، زاوية) الحقيقيين، وتتكمّم، والخطأ يروح للجيران
   **على القرص** — وعشان كده الحبيبات بتمشي مع المسار مش مع حدود الشاشة.
3. **درجات ← قطاعات.** `SectorComposer` بيحوّل كل درجة لقطاع خام 2352 بايت. في Audio DAO مفيش scramble
   ومفيش EDC/ECC، فالـ 2352 بايت كلها بتوصل للمشفّر زي ما هي.
4. **قطاعات ← درايف.** `RawDaoWriter` بينفّذ تسلسل MMC اللي فوق، و`SafeBufferMonitor` بيغذّيه، والتقدم
   والبَفر والسرعة المقترحة بترجع للواجهة.

تفاصيل أعمق في [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

---

<a id="download"></a>

## ⬇️ التحميل والتثبيت / Download

### الطريقة الأسهل: ملف exe جاهز

<div align="center">

**⬇️ [حمّل `DiscImageStudio-win32.exe` من صفحة الـ Releases](https://github.com/abd77o/DiscImageStudio/releases/latest)**

<br>

| الخطوة | اعمل إيه |
|:--:|---|
| ١ | دوس على زرار **Download** فوق، ونزّل `DiscImageStudio-win32.exe` (وفيه ملف `.sha256` للتأكد إن التحميل سليم) |
| ٢ | حطّه في أي فولدر — **مفيش تثبيت، مفيش Setup، مفيش registry** |
| ٣ | كليك يمين → **Run as administrator** (لازم — لأن الكتابة على الدرايف محتاجة صلاحية أدمن) |
| ٤ | لو Windows ظهر "Windows protected your PC" → **More info** → **Run anyway** (البرنامج مش موقّع بشهادة مدفوعة) |

</div>

### تأكيد سلامة الملف (اختياري)

```powershell
Get-FileHash .\DiscImageStudio-win32.exe -Algorithm SHA256
# وقارنها بملف DiscImageStudio-win32.exe.sha256 من نفس صفحة الإصدار
```

### أو ابنِها بنفسك من السورس

```bat
git clone https://github.com/abd77o/DiscImageStudio.git
cd DiscImageStudio
scripts\fetch_deps.bat       :: Dear ImGui + stb_image
build_vs2017.bat             :: -> build\Release\DiscImageStudio.exe
```

أو بـ CMake مباشرة: `cmake -S . -B build -A Win32 -T v141_xp` ثم `cmake --build build --config Release`.

<details>
<summary><strong>Linux / macOS — للاختبارات فقط (بدون واجهة وبدون حارق)</strong></summary>

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```
</details>

---

## 💻 المتطلبات / Requirements

| | |
|---|---|
| **النظام** | Windows 7 SP1 أو أحدث — بناء x86 static بيشتغل على ٦٤ بت كمان (الكود مصمم ومُهندس لـ 7 حتى 11) |
| **الرام** | أي حاجة — البرنامج نفسه بيشتغل في حدود ميجابايتات، ومصمم لأجهزة 512 ميجا |
| **الهاردوير** | حارق **CD-R / CD-RW** يدعم raw Disc-At-Once. **CD فقط** — معظم حوارق DVD ملهاش raw DAO |
| **الصور** | PNG / JPEG / BMP |
| **الصلاحيات** | Administrator (إلزامي للكتابة على الدرايف) |
| **الإنترنت** | مش مطلوب بعد التحميل |

---

## 🚀 الاستخدام / Usage

1. شغّل البرنامج **كـ Administrator**، اختار الحارق وسرعة منخفضة (**4×** بداية آمنة).
2. **تبويب المعايرة:** احرق السلّم على CD-RW، بص على الأسطوانة، سجّل الباند الأفتح والأغمق → بتتحفظ في `config.ini`.
3. **تبويب الصورة:** حمّل صورتك، ظبّط السطوع/التباين/جاما والدوران ونصف قطر الصورة، وشوف المعاينة.
   سيب **Test write** مفعّل لحد ما ينجح مرة.
4. **Burn:** التقدم ونسبة البَفر والسرعة المقترحة ظاهرين لايف. **إلغاء حرق حقيقي = تلقيح الأسطوانة.**

---

## 🛡️ السلامة وحدود الهاردوير / Safety

- **CD فقط.** الـ preflight بيرفض أي ميديا غير CD.
- **الحرق بيلغي الأسطوانة.** جرّب على CD-RW الأول.
- **الدقة محدودة بطبيعتها:** الخلايا ≈1.3 مم على المسار و1.6 µm عبره، والمشفّر بيمد البيانات على قطاع كامل —
  توقّع تدرّج ناعم في الاتجاه الشعاعي وتفاصيل أقل في الاتجاه المماسي.
- **Test write** مش مدعوم في حوارق حديثة كتير؛ لو درايفك رفضه، جرّب على CD-RW وسجّل `test_write_passed=1` في `config.ini`.
- القائمة الكاملة: [docs/HARDWARE_NOTES.md](docs/HARDWARE_NOTES.md).

---

## 📁 هيكل المشروع / Layout

| المسار | إيه فيه |
|---|---|
| `src/core` | هندسة القرص، تحميل الصور بالبث، محرك الديثرينج القطبي، `config.ini` |
| `src/burn` | scrambler، تركيب القطاعات، أوامر MMC، CUE sheet، حارس البَفر، كاتب DAO، جهاز SPTI |
| `src/calib` | مصادر المعايرة (سلّم / باندات متبادلة) |
| `src/gui` | خيط الحرق، منطق واجهة ImGui، مضيف Win32 + Direct3D 9 |
| `tests` | اختبارات الدرايف الوهمي (logic / gui / end-to-end) |
| `tools` | `dither_preview` — معاينة بدون واجهة |
| `scripts` · `docs` | تحميل الاعتماديات · المعمارية والبناء وملاحظات الهاردوير |

---

## 🧪 الاختبارات و CI / Tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
ctest --test-dir build --output-on-failure
```

تلات مجموعات: `test_burn_logic` (scrambler، تركيب القطاعات، كل CDB، حارس البَفر، سيناريوهات الحرق)،
`test_gui_logic` (الإعدادات، القص، دورة حياة المهمة، الإلغاء، قفل الحرق)، و`test_image_burn` (من BMP
لحد الكاتب على درايف وهمي). مفيش حاجة محتاجة حارق ولا أسطوانة ولا أدمن.

GitHub Actions بيشغّل الاختبارات على Linux (GCC + Clang) وبيبني الواجهة على Windows؛ وأي tag بيطلع
**إصدار منشور** فيه `DiscImageStudio-win32.exe` + checksum.

---

## 🗺️ خارطة الطريق / Roadmap

- [ ] **الواجهة الحديثة** زي التصميم المقترح (Start / Design / Preview / Etch / Manage + الوضع الداكن)
- [ ] مصفوفة تحقق على هاردوير حقيقي (درايف × ميديا × سرعة) + قالب تقرير جاهز
- [ ] معايرة أوتوماتيكية (قراءة السلّم بالفحص بدل العين)
- [ ] تعويض مسبق لتشويش CIRC لزيادة حِدّة الصورة
- [ ] وضع سطر أوامر لحرق مجموعة تصاميم دفعة واحدة
- [ ] التحقق من ثوابت Mode-1 في الـ CUE sheet (`kFormDataRaw`, `kFormLeadData`) ضد MMC

---

## 🤝 المساهمة / Contributing

أهم مساهمة ممكن تقدّمها دلوقتي هي **تقرير هاردوير**: موديل الدرايف، الفيرموير، نوع الأسطوانة، السرعة، النتيجة،
وصورة للأسطوانة. اقرأ [CONTRIBUTING.md](CONTRIBUTING.md) و[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

- 🐞 [بلّغ عن مشكلة](../../issues/new/choose) · 🔐 [سياسة الأمان](SECURITY.md) · 📋 [سجل التغييرات](CHANGELOG.md)

---

## 📄 الرخصة / License

[MIT](LICENSE) © 2026 **abd77o**.
طرف ثالث: **Dear ImGui** (MIT) و **stb_image** (public domain / MIT) — بيتحمّلوا وقت البناء ومش مرفوعين في الريبو.

## 📬 التواصل / Contact

- GitHub: [@abd77o](https://github.com/abd77o) · Email: [poi99poin@gmail.com](mailto:poi99poin@gmail.com)
- Releases: [github.com/abd77o/DiscImageStudio/releases](https://github.com/abd77o/DiscImageStudio/releases)

---

## English overview

**7Disc Image Studio** draws a picture into the **data side of a CD-R** using the disc's own pit pattern —
no label, no printer, no LightScribe drive. The laser is the pen.

- **One static `.exe`, zero runtime dependencies**, Dear ImGui + Direct3D 9 — runs on Windows 7 through 11,
  built for 512 MB machines. No installer, no redistributable, no network needed after download.
- **Polar dithering engine:** samples the image along the real spiral and error-diffuses in disc geometry
  (next cell on the track, same angle on following revolutions). Floyd–Steinberg / Jarvis–Judice–Ninke, 2–8 tones,
  constant memory.
- **Raw sector control:** hand-composed 2352-byte sectors (Audio DAO by default, ECMA-130 scrambler for Mode-1).
- **Full MMC burn path:** `SET CD SPEED` → `MODE SELECT` → `SEND CUE SHEET` → `WRITE(10)` from LBA −150 →
  `SYNCHRONIZE CACHE`, over SPTI, with buffer-underrun-free recording and a `READ BUFFER CAPACITY` watchdog.
- **16-step calibration ladder** to find the lightest/darkest tone pair for your drive and media.
- **Three hardware-free test suites** run against a mock drive on every push (Linux, GCC + Clang) plus a Windows build.

**Download:** [latest `DiscImageStudio-win32.exe`](https://github.com/abd77o/DiscImageStudio/releases/latest) ·
run it as Administrator. **Status: experimental** — it permanently destroys the disc it writes to, so start with a
CD-RW and read [docs/HARDWARE_NOTES.md](docs/HARDWARE_NOTES.md). Licensed under [MIT](LICENSE).
