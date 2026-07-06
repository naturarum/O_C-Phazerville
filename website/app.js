// o_C custom firmware configurator — no framework, no build step.
const REPO = "naturarum/O_C-Phazerville"; // change if you fork this

const state = {
  target: "T32",
  apps: new Set(),
  disabledApplets: new Set(),
  feats: new Set(),
};
let manifest = null;

async function init() {
  manifest = await (await fetch("build_manifest.json")).json();
  document.getElementById("repo-link").href = `https://github.com/${REPO}`;

  renderTargets();
  renderApps();
  renderApplets();
  renderFeatures();
  restoreFromHash();
  update();
}

function renderTargets() {
  const el = document.getElementById("targets");
  el.innerHTML = manifest.targets.map(t => `
    <label class="row">
      <input type="radio" name="target" value="${t.id}"
             ${t.id === state.target ? "checked" : ""}>
      <span>${t.label}</span>
    </label>`).join("");
  el.addEventListener("change", e => {
    state.target = e.target.value;
    update();
  });
}

function renderApps() {
  const el = document.getElementById("apps");
  el.innerHTML = manifest.apps.map(a => `
    <label class="card ${a.t32 ? "" : "unavailable"}">
      <input type="checkbox" data-kind="app" value="${a.id}"
             ${a.t32 ? "" : "disabled"}>
      <div><strong>${a.name}</strong><br><small>${a.desc}</small></div>
    </label>`).join("");
}

function renderApplets() {
  const el = document.getElementById("applets");
  const byCat = {};
  for (const a of manifest.applets) {
    if (!a.t32) continue;
    const cat = a.categories[0] || "other";
    (byCat[cat] = byCat[cat] || []).push(a);
  }
  document.getElementById("applet-count").textContent =
    Object.values(byCat).flat().length;
  el.innerHTML = Object.entries(byCat).sort().map(([cat, list]) => `
    <div class="cat">
      <h3>${cat}</h3>
      ${list.map(a => `
        <label class="row">
          <input type="checkbox" data-kind="applet" value="${a.id}" checked>
          <span>${a.id}${a.gated_by ? ` <small>(needs ${a.gated_by})</small>` : ""}</span>
        </label>`).join("")}
    </div>`).join("");
}

function renderFeatures() {
  const el = document.getElementById("features");
  el.innerHTML = manifest.features.map(f => `
    <label class="card">
      <input type="checkbox" data-kind="feat" value="${f.id}">
      <div><strong>${f.name}</strong><br><small>${f.desc}</small></div>
    </label>`).join("");
}

document.addEventListener("change", e => {
  const kind = e.target.dataset && e.target.dataset.kind;
  if (!kind) return;
  const v = e.target.value;
  if (kind === "app") e.target.checked ? state.apps.add(v) : state.apps.delete(v);
  if (kind === "feat") e.target.checked ? state.feats.add(v) : state.feats.delete(v);
  if (kind === "applet")
    e.target.checked ? state.disabledApplets.delete(v) : state.disabledApplets.add(v);
  update();
});

function configLine() {
  const parts = [`oc-build/1 target=${state.target}`];
  if (state.apps.size)
    parts.push("apps=" + [...state.apps].map(a => "+" + a).join(","));
  if (state.disabledApplets.size)
    parts.push("applets=" + [...state.disabledApplets].map(a => "-" + a).join(","));
  if (state.feats.size)
    parts.push("feats=" + [...state.feats].map(f => "+" + f).join(","));
  return parts.join(" ");
}

function update() {
  const line = configLine();
  document.getElementById("config-line").textContent = line;
  const url = `https://github.com/${REPO}/issues/new?template=custom-build.yml&config=${encodeURIComponent(line)}`;
  document.getElementById("open-issue").href = url;
  history.replaceState(null, "", "#c=" + btoa(line));
}

function restoreFromHash() {
  if (!location.hash.startsWith("#c=")) return;
  try {
    const line = atob(location.hash.slice(3));
    const m = line.match(/target=(\S+)/);
    if (m) state.target = m[1];
    (line.match(/apps=(\S+)/) || [, ""])[1].split(",").forEach(t => {
      if (t.startsWith("+")) state.apps.add(t.slice(1));
    });
    (line.match(/applets=(\S+)/) || [, ""])[1].split(",").forEach(t => {
      if (t.startsWith("-")) state.disabledApplets.add(t.slice(1));
    });
    (line.match(/feats=(\S+)/) || [, ""])[1].split(",").forEach(t => {
      if (t.startsWith("+")) state.feats.add(t.slice(1));
    });
    // re-sync UI
    document.querySelectorAll("input[data-kind=app]").forEach(i =>
      i.checked = state.apps.has(i.value));
    document.querySelectorAll("input[data-kind=applet]").forEach(i =>
      i.checked = !state.disabledApplets.has(i.value));
    document.querySelectorAll("input[data-kind=feat]").forEach(i =>
      i.checked = state.feats.has(i.value));
    document.querySelectorAll("input[name=target]").forEach(i =>
      i.checked = i.value === state.target);
  } catch (e) { /* bad hash, ignore */ }
}

document.getElementById("copy-config").addEventListener("click", () =>
  navigator.clipboard.writeText(configLine()));
document.getElementById("copy-link").addEventListener("click", () =>
  navigator.clipboard.writeText(location.href));

init();
