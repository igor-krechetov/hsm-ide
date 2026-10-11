#!/usr/bin/env python3
"""Render the hsm-ide TRLC requirements to a single self-contained HTML file.

Loads requirements/ (the .rsl model plus the .trlc requirement files) via the
TRLC Python API and produces a filterable HTML view of the software
requirements, grouped under their document SECTIONS as a collapsible
TREE-TABLE that preserves the specification's document order and nesting (the
hierarchy from requirements/requirements.pu).

Adapted from hsmcpp's scripts/requirements/render_html.py, reduced for hsm-ide:
hsm-ide has only Software_Requirements (no System tab, no glossary, no
parent/derived links, no code/test traceability). The columns are UID, Title,
Status and Description, with a status filter and status-based color coding that
mirror the .pu <<class>> palette.

The page has two tabs sharing one derived data model:
  * Requirements  - the collapsible tree-table, now with a COMPUTED roll-up
                    progress bar on each section header (see STATUS_WEIGHT).
  * Status Graph  - a zoomable D3 sunburst of the same hierarchy, arc size by
                    leaf count, color by status / computed roll-up.
D3 v7 is vendored next to this script (d3.v7.min.js) and INLINED into the
output so the file opens offline (file://) with no network dependency.

Usage:
    python3 scripts/requirements/render_html.py [OUTPUT_HTML]

Default output: build/requirements/hsm_ide_requirements.html
"""
import json
import sys
from pathlib import Path

from trlc.errors import Message_Handler
from trlc.trlc import Source_Manager

REPO = Path(__file__).resolve().parents[2]
REQ_DIR = REPO / "requirements"
# Vendored D3 v7 (additive file next to this script); inlined so the output is
# self-contained and opens offline with no CDN.
D3_PATH = Path(__file__).resolve().parent / "d3.v7.min.js"


def field(obj, name):
    val = obj.to_python_dict().get(name)
    if val is None:
        return None
    if hasattr(val, "name"):
        return val.name.split(".")[-1]
    return str(val)


def section_path(obj):
    """Return the list of section titles enclosing a record, outermost first.

    TRLC stores obj.section as a list of Section nodes (outer..inner) or None.
    """
    secs = getattr(obj, "section", None)
    if not secs:
        return []
    return [s.name for s in secs]


def load():
    mh = Message_Handler()
    sm = Source_Manager(mh)
    sm.register_directory(str(REQ_DIR))
    symbols = sm.process()
    if symbols is None:
        print("TRLC processing failed", file=sys.stderr)
        sys.exit(1)
    return symbols


# ---------------------------------------------------------------------------
# Roll-up weighting (DOCUMENTED, see FEATURE 1).
#
# Parent/section completion is COMPUTED from its descendant LEAF requirements,
# never read from any parent record's stored `status`. Stored `status` values
# are left untouched; a container record (e.g. REQ-100 Partial) keeps its
# authored status, but the progress bar shown on its section header is derived.
#
# A LEAF requirement is one that does NOT itself open a child section (i.e. its
# uid is not used as a section-header uid). Container records are therefore
# excluded from the roll-up population so they are not double-counted with the
# leaves beneath them.
#
# Numerator weighting (fractional credit for in-flight work):
#   Done = 1.0, Partial = 0.5, InProgress = 0.5, Todo = 0.0
# Denominator = count of (Done + InProgress + Todo + Partial) leaves.
#
# PrioLow leaves are INTENTIONALLY DEFERRED: they are excluded from the
# denominator and reported separately as "+N deferred". A section whose only
# descendants are PrioLow has denominator 0 and is shown as neutral "deferred"
# rather than 0% / NaN.
# ---------------------------------------------------------------------------
STATUS_WEIGHT = {"Done": 1.0, "Partial": 0.5, "InProgress": 0.5, "Todo": 0.0}


def section_uid(section_title):
    """Return the leading UID of a section title string "UID: Title"."""
    return section_title.split(":", 1)[0].strip()


def rollup_stats(leaf_statuses):
    """Compute the roll-up for a list of leaf statuses.

    Returns a dict with the completion ratio (or None when fully deferred),
    the weighted done/total used for the "done/total" label and the deferred
    (PrioLow) count. `done` is reported as the integer count of fully-Done
    leaves so the "(done/total)" label matches a hand count of Done records.
    """
    denom = 0
    weighted = 0.0
    done = 0
    deferred = 0
    for st in leaf_statuses:
        if st == "PrioLow":
            deferred += 1
            continue
        denom += 1
        weighted += STATUS_WEIGHT.get(st, 0.0)
        if st == "Done":
            done += 1
    ratio = (weighted / denom) if denom else None
    return {
        "ratio": ratio,
        "done": done,
        "total": denom,
        "deferred": deferred,
    }


def build_tree(sw_reqs):
    """Build a hierarchical tree from the records' section paths.

    Each node mirrors one section level; its `children` are nested sections and
    its leaf requirements. Section-header records (a record whose own section
    path ends with its own uid) are attached to their node as `headerStatus`
    rather than counted as leaves. Roll-up stats are computed bottom-up over the
    LEAF requirements only (see STATUS_WEIGHT doc above).

    Returns (root_node, None); the second element is kept for symmetry with the
    finalize step and is unused.
    """
    root = {"uid": "", "title": "", "key": "", "children": {},
            "leaves": [], "headerStatus": None, "order": -1}

    def ensure_path(path):
        node = root
        prefix = []
        for sec in path:
            prefix.append(sec)
            key = " \u203a ".join(prefix)
            child = node["children"].get(key)
            if child is None:
                child = {"uid": section_uid(sec),
                         "title": sec.split(":", 1)[1].strip()
                         if ":" in sec else sec,
                         "key": key, "children": {}, "leaves": [],
                         "headerStatus": None, "order": len(node["children"])}
                node["children"][key] = child
            node = child
        return node

    for r in sw_reqs:
        node = ensure_path(r["section"])
        is_header = (r["section"]
                     and section_uid(r["section"][-1]) == r["uid"])
        if is_header:
            # The header record's own section path ends with its own uid, so
            # `node` is exactly the section node it heads. Store its authored
            # status for reference (NOT used in the computed roll-up).
            node["headerStatus"] = r["status"]
        else:
            node["leaves"].append(r)
    return root, None


def finalize_tree(node):
    """Convert the dict-tree to an ordered, roll-up-annotated structure.

    Returns (node_dict, leaf_statuses) where node_dict carries `rollup` and an
    ordered `children` list suitable for JSON serialization / the sunburst.
    """
    leaf_statuses = [leaf["status"] for leaf in node["leaves"]]
    children_out = []
    for child in sorted(node["children"].values(), key=lambda c: c["order"]):
        child_out, child_statuses = finalize_tree(child)
        children_out.append(child_out)
        leaf_statuses.extend(child_statuses)
    stats = rollup_stats(leaf_statuses)
    # Keep only the fields the sunburst/tooltip needs for leaves to avoid
    # duplicating the full description text already carried in `sw`.
    leaves_out = [{"uid": leaf["uid"], "title": leaf["title"],
                   "status": leaf["status"],
                   "hasDetails": bool(leaf.get("details"))}
                  for leaf in node["leaves"]]
    out = {
        "uid": node["uid"],
        "title": node["title"],
        "key": node["key"],
        "headerStatus": node["headerStatus"],
        "rollup": stats,
        "children": children_out,
        "leaves": leaves_out,
    }
    return out, leaf_statuses


def build_data():
    symbols = load()
    sw_reqs = []
    # Preserve document order (iter_record_objects yields in file order) so the
    # section grouping matches the specification's reading order.
    for obj in symbols.iter_record_objects():
        if obj.n_typ.name != "Software_Requirement":
            continue
        sw_reqs.append({
            "uid": field(obj, "uid") or "",
            "title": field(obj, "title") or "",
            "description": field(obj, "description") or "",
            "status": field(obj, "status") or "",
            # Optional long-form specification (Markup_String). None when the
            # requirement has no details so the row renders no disclosure.
            "details": field(obj, "details"),
            "section": section_path(obj),
        })
    tree_root, _ = build_tree(sw_reqs)
    tree, _ = finalize_tree(tree_root)
    # Map section key -> roll-up so the tree-table can render progress bars on
    # each section header without re-deriving the hierarchy in JS.
    rollup_map = {}

    def collect(node):
        if node["key"]:
            rollup_map[node["key"]] = node["rollup"]
        for child in node["children"]:
            collect(child)

    collect(tree)
    return {"sw": sw_reqs, "tree": tree, "rollup": rollup_map}


# ---------------------------------------------------------------------------
# HTML template.
#
# NOTE: this template is filled with plain str.replace of the __TOKEN__
# placeholders (not str.format) specifically because the inlined D3 source and
# the embedded JSON both contain many '{' / '}' characters that would otherwise
# have to be escaped. The CSS/JS below therefore use single braces.
# ---------------------------------------------------------------------------
TEMPLATE = """<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<title>hsm-ide Requirements</title>
<style>
:root{--bg:#14161c;--surface:#1c1f27;--card:#242833;--accent:#4f9cf9;--text:#e6e8ec;--muted:#9aa0ac;}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',system-ui,sans-serif;background:var(--bg);color:var(--text);padding:2rem;line-height:1.5}
h1{color:var(--accent)}.sub{color:var(--muted);margin-bottom:1.5rem}
.stats{display:flex;gap:1rem;margin-bottom:1.5rem;flex-wrap:wrap}
.stat{background:var(--surface);border-radius:8px;padding:.8rem 1.4rem;border-left:4px solid var(--accent)}
.stat .n{font-size:1.8rem;font-weight:700;color:var(--accent)}.stat .l{color:var(--muted);font-size:.8rem}
.tabs{display:flex;gap:.5rem;margin-bottom:1.2rem;border-bottom:1px solid #2b3547}
.tab{background:none;border:none;border-bottom:3px solid transparent;color:var(--muted);padding:.6rem 1.1rem;cursor:pointer;font-size:.95rem;font-weight:600}
.tab:hover{color:var(--text)}
.tab.active{color:var(--accent);border-bottom-color:var(--accent)}
.panel{display:none}
.panel.active{display:block}
.controls{display:flex;gap:1rem;margin-bottom:1rem;flex-wrap:wrap;align-items:center}
input[type=search],select{background:var(--surface);border:1px solid #333;border-radius:6px;padding:.5rem .8rem;color:var(--text)}
input[type=search]{width:320px}
.controls button{background:var(--surface);border:1px solid #333;border-radius:6px;padding:.5rem .8rem;color:var(--muted);cursor:pointer}
.controls button:hover{color:var(--text);border-color:var(--accent)}
table{width:100%;border-collapse:collapse;background:var(--surface);border-radius:8px;overflow:hidden}
thead th{position:sticky;top:0;z-index:2}
th{background:#2b3547;padding:.85rem .7rem;text-align:left;font-size:.72rem;letter-spacing:.6px;text-transform:uppercase;color:#cdd3dd;font-weight:700;border-bottom:3px solid var(--accent);box-shadow:0 2px 6px rgba(0,0,0,.35)}
td{padding:.6rem .7rem;border-top:1px solid #23252c;font-size:.9rem;vertical-align:top}
tr.req:hover{background:rgba(79,156,249,.06)}
tr.sec-row td{cursor:pointer;font-weight:600;color:var(--accent);padding:.5rem .7rem;
  background:linear-gradient(90deg,rgba(79,156,249,.14),rgba(79,156,249,.03));
  border-top:1px solid #23252c;border-left:3px solid var(--accent)}
tr.sec-row:hover td{background:linear-gradient(90deg,rgba(79,156,249,.22),rgba(79,156,249,.06))}
tr.sec-row[data-depth="1"] td{color:var(--text);border-left-color:#3d6ea5;
  background:linear-gradient(90deg,rgba(79,156,249,.08),rgba(79,156,249,.02));font-size:.86rem}
tr.sec-row[data-depth="2"] td{color:var(--muted);border-left-color:#31415a;background:rgba(79,156,249,.04)}
tr.sec-row .chev{display:inline-block;width:1rem;color:var(--accent)}
.sec-head{display:flex;align-items:center;gap:.6rem}
.sec-title{flex:0 0 auto}
.sec-bar{flex:0 0 160px;height:11px;border-radius:6px;background:#2b3547;overflow:hidden;position:relative}
.sec-bar>span{display:block;height:100%}
.sec-pct{flex:0 0 auto;font-weight:400;font-size:.78rem;color:var(--muted);white-space:nowrap}
.sec-deferred{flex:0 0 auto;font-weight:400;font-size:.74rem;color:#87ceeb;white-space:nowrap}
.status{display:inline-block;padding:.15rem .55rem;border-radius:10px;font-size:.72rem;font-weight:700;color:#14161c}
.status.Done{background:#90ee90}
.status.InProgress{background:#ffff00}
.status.Todo{background:#fa8072}
.status.Partial{background:#fffacd}
.status.PrioLow{background:#87ceeb}
.desc{max-width:720px}
.none{color:var(--muted)}
.det-toggle{display:inline-block;margin-top:.4rem;color:var(--accent);cursor:pointer;font-size:.78rem;font-weight:600;user-select:none}
.det-toggle:hover{text-decoration:underline}
.det-toggle .chev{display:inline-block;width:.9rem}
tr.det-row td{background:#191c24;border-top:none;padding:.2rem .7rem .8rem}
tr.det-row.hidden{display:none}
.det-body{max-width:860px;font-size:.84rem;color:var(--text);background:var(--card);
  border-left:3px solid var(--accent);border-radius:0 6px 6px 0;padding:.7rem 1rem}
.det-body h4{font-size:.76rem;text-transform:uppercase;letter-spacing:.5px;color:var(--accent);
  margin:.7rem 0 .3rem}
.det-body h4:first-child{margin-top:0}
.det-body ul,.det-body ol{margin:.2rem 0 .2rem 1.3rem}
.det-body li{margin:.15rem 0}
.det-body p{margin:.3rem 0}
.det-body code{background:#0d0f14;border-radius:4px;padding:.05rem .3rem;font-size:.82rem}
/* Status Graph tab */
.graph-wrap{display:flex;gap:2rem;flex-wrap:wrap;align-items:flex-start}
#sunburst{background:var(--surface);border-radius:8px}
.legend{background:var(--surface);border-radius:8px;padding:1rem 1.2rem;min-width:240px}
.legend h3{font-size:.8rem;text-transform:uppercase;letter-spacing:.6px;color:#cdd3dd;margin-bottom:.6rem}
.legend .row{display:flex;align-items:center;gap:.55rem;margin:.3rem 0;font-size:.85rem;color:var(--text)}
.legend .sw{width:16px;height:16px;border-radius:4px;flex:0 0 auto;border:1px solid rgba(0,0,0,.3)}
.legend .grad{height:14px;border-radius:4px;margin:.4rem 0;
  background:linear-gradient(90deg,#fa8072,#fffacd,#90ee90)}
.legend .scale{display:flex;justify-content:space-between;font-size:.72rem;color:var(--muted)}
.legend p{font-size:.78rem;color:var(--muted);margin-top:.6rem}
.g-hint{color:var(--muted);font-size:.82rem;margin-bottom:.8rem}
#tooltip{position:fixed;pointer-events:none;background:#0d0f14;border:1px solid var(--accent);
  border-radius:6px;padding:.5rem .7rem;font-size:.82rem;color:var(--text);max-width:300px;
  opacity:0;transition:opacity .1s;z-index:10;box-shadow:0 4px 14px rgba(0,0,0,.5)}
#tooltip .t-uid{color:var(--accent);font-weight:700}
#tooltip .t-sub{color:var(--muted);font-size:.76rem;margin-top:.2rem}
</style></head><body>
<h1>hsm-ide Requirements Specification</h1>
<p class="sub">Generated from TRLC (BMW format) &mdash; __N_SW__ software requirements. Status tracks implementation state (Done = implemented).</p>
<div class="stats">
 <div class="stat"><div class="n">__N_SW__</div><div class="l">Requirements</div></div>
 <div class="stat"><div class="n">__N_DONE__</div><div class="l">Done</div></div>
 <div class="stat"><div class="n">__N_INPROGRESS__</div><div class="l">In Progress</div></div>
 <div class="stat"><div class="n">__N_PARTIAL__</div><div class="l">Partial</div></div>
 <div class="stat"><div class="n">__N_TODO__</div><div class="l">Todo</div></div>
 <div class="stat"><div class="n">__N_PRIOLOW__</div><div class="l">Prio Low</div></div>
</div>

<div class="tabs">
 <button class="tab active" data-panel="p-tree">Requirements</button>
 <button class="tab" data-panel="p-graph">Status Graph</button>
</div>

<section id="p-tree" class="panel active">
 <div class="controls">
  <input type="search" id="q" placeholder="Search requirements...">
  <select id="fstatus">
   <option value="">All statuses</option>
   <option value="Done">Done</option>
   <option value="InProgress">InProgress</option>
   <option value="Todo">Todo</option>
   <option value="Partial">Partial</option>
   <option value="PrioLow">PrioLow</option>
  </select>
  <button id="expand">Expand all</button><button id="collapse">Collapse all</button>
 </div>
 <table><thead><tr><th>UID</th><th>Title</th><th>Status</th><th>Description</th></tr></thead><tbody id="b-sw"></tbody></table>
</section>

<section id="p-graph" class="panel">
 <p class="g-hint">Zoomable sunburst of the requirement hierarchy. Arc size = number of leaf requirements in the branch. Click an arc to zoom in; click the center to zoom back out. Inner rings are colored by the <strong>computed</strong> roll-up (not an authored status).</p>
 <div class="graph-wrap">
  <svg id="sunburst" width="680" height="680"></svg>
  <div class="legend">
   <h3>Leaf status</h3>
   <div class="row"><span class="sw" style="background:#90ee90"></span>Done</div>
   <div class="row"><span class="sw" style="background:#ffff00"></span>InProgress</div>
   <div class="row"><span class="sw" style="background:#fffacd"></span>Partial</div>
   <div class="row"><span class="sw" style="background:#fa8072"></span>Todo</div>
   <div class="row"><span class="sw" style="background:#87ceeb"></span>PrioLow (deferred)</div>
   <h3 style="margin-top:1rem">Section roll-up</h3>
   <div class="grad"></div>
   <div class="scale"><span>0%</span><span>50%</span><span>100%</span></div>
   <p>Inner arcs use the completion roll-up color. PrioLow leaves are excluded from the roll-up denominator (deferred) and shown in blue.</p>
  </div>
 </div>
</section>

<div id="tooltip"></div>

<script>
__D3_SOURCE__
</script>
<script>
const D=__DATA_JSON__;
const SW_COLS=4;
const STATUS_COLOR={Done:'#90ee90',InProgress:'#ffff00',Todo:'#fa8072',Partial:'#fffacd',PrioLow:'#87ceeb'};

function statusBadge(s){return s?`<span class="status ${s}">${s}</span>`:'<span class="none">&mdash;</span>';}

// Escape text so it is rendered literally in HTML. The details content holds
// `<script>`, `<onentry>`, `<`, `>` and backticks straight from the acceptance
// criteria; without this they could inject tags or break the page.
function esc(s){
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;')
    .replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

// Render the long-form details (plain markdown-ish text from Markup_String) to
// safe HTML: ALWAYS escape first, then turn `code` spans, bullet lines
// (- / *), numbered lines (1.) and "Heading:" lines into formatted markup.
// Blank lines separate blocks; everything else becomes a paragraph.
function renderDetails(text){
  const inline=s=>esc(s).replace(/`([^`]+)`/g,'<code>$1</code>');
  const lines=String(text).split(/\\r?\\n/);
  let html='',list=null; // list = 'ul' | 'ol' | null
  const closeList=()=>{ if(list){ html+=`</${list}>`; list=null; } };
  for(const raw of lines){
    const line=raw.replace(/\\s+$/,'');
    if(!line.trim()){ closeList(); continue; }
    let m;
    if((m=line.match(/^\\s*[-*]\\s+(.*)$/))){
      if(list!=='ul'){ closeList(); html+='<ul>'; list='ul'; }
      html+=`<li>${inline(m[1])}</li>`;
    } else if((m=line.match(/^\\s*\\d+\\.\\s+(.*)$/))){
      if(list!=='ol'){ closeList(); html+='<ol>'; list='ol'; }
      html+=`<li>${inline(m[1])}</li>`;
    } else if((m=line.match(/^([A-Z][A-Za-z /-]{2,40}):\\s*$/))){
      closeList(); html+=`<h4>${inline(m[1])}</h4>`;
    } else if((m=line.match(/^([A-Z][A-Za-z /-]{2,40}):\\s+(.*)$/))){
      closeList(); html+=`<p><strong>${inline(m[1])}:</strong> ${inline(m[2])}</p>`;
    } else {
      closeList(); html+=`<p>${inline(line)}</p>`;
    }
  }
  closeList();
  return html;
}

// Build a stable section key from a path so rows can be toggled together.
function secKey(path){return path.join(' \\u203a ');}

// Roll-up color: red (low) -> amber (mid) -> green (high). Mirrors the .pu
// palette extremes (todo=Salmon .. done=LightGreen) with a mid amber band and
// matches the Feature-1 progress-bar scheme used in the tree-table.
function rollupColor(ratio){
  if(ratio===null||ratio===undefined) return '#87ceeb'; // deferred / neutral
  const stops=[[0,[250,128,114]],[0.5,[255,250,205]],[1,[144,238,144]]];
  let a=stops[0],b=stops[stops.length-1];
  for(let i=0;i<stops.length-1;i++){ if(ratio>=stops[i][0]&&ratio<=stops[i+1][0]){a=stops[i];b=stops[i+1];break;} }
  const t=(b[0]-a[0])?(ratio-a[0])/(b[0]-a[0]):0;
  const c=k=>Math.round(a[1][k]+(b[1][k]-a[1][k])*t);
  return `rgb(${c(0)},${c(1)},${c(2)})`;
}

function pctLabel(ru){
  if(ru.total===0) return 'deferred';
  return Math.round(ru.ratio*100)+'% ('+ru.done+'/'+ru.total+')';
}

function rowSw(r){
  // Short description stays as the Description column. When the requirement
  // carries optional long-form `details`, add a disclosure toggle that reveals
  // a separate, formatted row below it (hidden by default).
  let descCell=`<td class="desc">${esc(r.description)}`;
  let detRow='';
  if(r.details){
    descCell+=`<span class="det-toggle" onclick="toggleDetails(this)">`
      +`<span class="chev">&#9656;</span>Details</span>`;
    detRow=`<tr class="det-row hidden" data-sec="${secKey(r.section)}">`
      +`<td colspan="${SW_COLS}"><div class="det-body">${renderDetails(r.details)}</div></td></tr>`;
  }
  descCell+='</td>';
  return `<tr class="req" data-id="${r.uid}" data-sec="${secKey(r.section)}"><td><strong>${r.uid}</strong></td><td>${esc(r.title)}</td><td>${statusBadge(r.status)}</td>${descCell}</tr>`+detRow;
}

// Reveal / hide the details row that immediately follows a requirement row.
function toggleDetails(toggle){
  const reqRow=toggle.closest('tr');
  const detRow=reqRow.nextElementSibling;
  if(!detRow||!detRow.classList.contains('det-row')) return;
  const hidden=detRow.classList.toggle('hidden');
  toggle.querySelector('.chev').innerHTML = hidden ? '&#9656;' : '&#9662;';
}

// Section header cell: title + COMPUTED roll-up progress bar + "X% (done/total)"
// + "+N deferred". The roll-up comes from D.rollup (section key -> stats); when
// a key is absent (filtered view with no matching leaves) the bar is omitted.
function secHeadCell(sub, d){
  const key=secKey(sub);
  const ru=D.rollup[key];
  let bar='';
  if(ru){
    const pct=ru.total?Math.round(ru.ratio*100):0;
    const col=rollupColor(ru.total?ru.ratio:null);
    const deferred=ru.deferred?`<span class="sec-deferred">+${ru.deferred} deferred</span>`:'';
    bar=`<span class="sec-bar"><span style="width:${pct}%;background:${col}"></span></span>`
       +`<span class="sec-pct">${pctLabel(ru)}</span>${deferred}`;
  }
  return `<td colspan="${SW_COLS}"><span class="sec-head" style="padding-left:${d*1.2}rem">`
       +`<span class="sec-title"><span class="chev">&#9662;</span>${sub[d]}</span>${bar}</span></td>`;
}

// Render a list of requirements as a tree-table: emit a collapsible header row
// whenever the section path changes, then the requirement rows beneath it.
function renderTree(items, rowFn){
  let html='', prev=[];
  items.forEach(r=>{
    const path=r.section||[];
    // find first index where path diverges from prev
    let i=0; while(i<path.length && i<prev.length && path[i]===prev[i]) i++;
    for(let d=i; d<path.length; d++){
      const sub=path.slice(0,d+1); const key=secKey(sub);
      html+=`<tr class="sec-row" data-key="${key}" data-depth="${d}" onclick="toggleSec(this)">${secHeadCell(sub,d)}</tr>`;
    }
    prev=path;
    html+=rowFn(r);
  });
  return html;
}

// Collapse state is the SINGLE source of truth: each section header carries the
// `collapsed` class when its own subtree is folded. Visibility is never set
// ad-hoc; it is always DERIVED by refreshTree() from the set of collapsed
// headers. A row is visible iff none of its ancestor sections are collapsed.
//
// refreshTree walks all rows in document order keeping a stack of the depths of
// currently-open (not-collapsed) ancestor sections. `collapsedAt` is the depth
// of the shallowest collapsed ancestor in effect (Infinity when none); any row
// deeper than it is hidden.
function refreshTree(){
  let collapsedAt=Infinity;
  document.querySelectorAll('#b-sw tr').forEach(r=>{
    if(r.classList.contains('sec-row')){
      const d=+r.dataset.depth;
      // Reaching a header at or above the collapsed boundary ends that region.
      if(d<=collapsedAt) collapsedAt=Infinity;
      // The header itself is visible unless nested under a collapsed ancestor.
      r.style.display = d>collapsedAt ? 'none' : '';
      // Keep the chevron in sync with the stored collapsed state.
      const chev=r.querySelector('.chev');
      if(chev) chev.innerHTML = r.classList.contains('collapsed') ? '&#9656;' : '&#9662;';
      // If this (visible) header is collapsed, it hides everything deeper.
      if(r.style.display!=='none' && r.classList.contains('collapsed') && d<collapsedAt)
        collapsedAt=d;
    } else {
      // Requirement row: visible only when no collapsed ancestor is in effect.
      r.style.display = collapsedAt===Infinity ? '' : 'none';
    }
  });
}
// Toggle one header's collapsed state, then re-derive all visibility.
function toggleSec(hdr){
  hdr.classList.toggle('collapsed');
  refreshTree();
}
// Collapse/expand every section, then re-derive visibility in one pass.
function setAll(collapse){
  document.querySelectorAll('#b-sw tr.sec-row').forEach(h=>{
    h.classList.toggle('collapsed', collapse);
  });
  refreshTree();
}

function apply(){
  const q=document.getElementById('q').value.toLowerCase();
  const st=document.getElementById('fstatus').value;
  const match=r=>(!q||JSON.stringify(r).toLowerCase().includes(q))&&(!st||r.status===st);
  document.getElementById('b-sw').innerHTML=renderTree(D.sw.filter(match), rowSw);
}
document.getElementById('q').addEventListener('input',apply);
document.getElementById('fstatus').addEventListener('change',apply);
document.getElementById('expand').addEventListener('click',()=>setAll(false));
document.getElementById('collapse').addEventListener('click',()=>setAll(true));
apply();

// --- Tabs -----------------------------------------------------------------
let sunburstDrawn=false;
document.querySelectorAll('.tab').forEach(t=>{
  t.addEventListener('click',()=>{
    document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));
    document.querySelectorAll('.panel').forEach(x=>x.classList.remove('active'));
    t.classList.add('active');
    document.getElementById(t.dataset.panel).classList.add('active');
    if(t.dataset.panel==='p-graph' && !sunburstDrawn){ drawSunburst(); sunburstDrawn=true; }
  });
});

// --- D3 zoomable sunburst --------------------------------------------------
// The hierarchy is D.tree (section nesting + compact leaves). Each leaf counts
// as 1 so arc angular size = number of leaf requirements in a branch. Leaf arcs
// are colored by status; inner (section) arcs by the COMPUTED roll-up color.
function drawSunburst(){
  // Flatten the derived tree into a d3.hierarchy-friendly shape: section nodes
  // keep children; leaves become value-1 nodes carrying their status.
  function toNode(n){
    const kids=[];
    (n.children||[]).forEach(c=>kids.push(toNode(c)));
    (n.leaves||[]).forEach(l=>kids.push({name:l.uid,title:l.title,status:l.status,hasDetails:l.hasDetails,leaf:true}));
    return {name:n.uid||'REQ',title:n.title||'All requirements',rollup:n.rollup,key:n.key,children:kids};
  }
  const rootData=toNode(D.tree);

  const svg=d3.select('#sunburst');
  const width=+svg.attr('width');
  const radius=width/2;
  svg.attr('viewBox',`${-radius} ${-radius} ${width} ${width}`).style('font','10px sans-serif');
  svg.selectAll('*').remove();
  const g=svg.append('g');

  const root=d3.hierarchy(rootData)
    .sum(d=>d.leaf?1:0)
    .sort((a,b)=>b.value-a.value);
  const partition=d3.partition().size([2*Math.PI, root.height+1]);
  partition(root);
  root.each(d=>d.current=d);

  const maxRadius=radius-10;
  const arc=d3.arc()
    .startAngle(d=>d.x0)
    .endAngle(d=>d.x1)
    .padAngle(d=>Math.min((d.x1-d.x0)/2,0.004))
    .padRadius(maxRadius*1.5)
    .innerRadius(d=>d.y0*maxRadius/(root.height+1))
    .outerRadius(d=>Math.max(d.y0*maxRadius/(root.height+1), d.y1*maxRadius/(root.height+1)-1));

  function colorOf(d){
    if(d.data.leaf) return STATUS_COLOR[d.data.status]||'#555';
    const ru=d.data.rollup;
    return ru?rollupColor(ru.total?ru.ratio:null):'#3d6ea5';
  }

  const tooltip=d3.select('#tooltip');
  function showTip(ev,d){
    const ru=d.data.rollup;
    let sub;
    if(d.data.leaf){ sub=`status: ${d.data.status}`+(d.data.hasDetails?' &middot; has details':''); }
    else { sub=ru?`roll-up: ${pctLabel(ru)}${ru.deferred?' (+'+ru.deferred+' deferred)':''}`:'section'; }
    tooltip.html(`<div class="t-uid">${d.data.name}</div><div>${d.data.title||''}</div><div class="t-sub">${sub}</div>`)
      .style('left',(ev.clientX+14)+'px').style('top',(ev.clientY+14)+'px').style('opacity',1);
  }
  function moveTip(ev){ tooltip.style('left',(ev.clientX+14)+'px').style('top',(ev.clientY+14)+'px'); }
  function hideTip(){ tooltip.style('opacity',0); }

  const path=g.append('g').selectAll('path')
    .data(root.descendants().slice(1))
    .join('path')
      .attr('fill',colorOf)
      .attr('fill-opacity',d=>arcVisible(d.current)?(d.children?0.85:0.95):0)
      .attr('pointer-events',d=>arcVisible(d.current)?'auto':'none')
      .attr('d',d=>arc(d.current))
      .style('cursor','pointer');

  path.on('mouseover',showTip).on('mousemove',moveTip).on('mouseout',hideTip);
  path.filter(d=>d.children).on('click',clicked);

  const label=g.append('g')
      .attr('pointer-events','none').attr('text-anchor','middle').style('user-select','none')
    .selectAll('text')
    .data(root.descendants().slice(1))
    .join('text')
      .attr('dy','0.35em').attr('fill','#14161c').attr('font-weight','600')
      .attr('fill-opacity',d=>+labelVisible(d.current))
      .attr('transform',d=>labelTransform(d.current))
      .text(d=>d.data.name);

  const parentCircle=g.append('circle')
      .datum(root)
      .attr('r',maxRadius/(root.height+1))
      .attr('fill','#2b3547').attr('stroke','#4f9cf9')
      .attr('pointer-events','all').style('cursor','pointer')
      .on('click',clicked);
  const centerText=g.append('text')
      .attr('text-anchor','middle').attr('dy','0.35em')
      .attr('fill','#e6e8ec').attr('font-weight','700').attr('font-size','12px')
      .attr('pointer-events','none').text('REQ-000');

  function clicked(event,p){
    parentCircle.datum(p.parent||root);
    centerText.text((p.parent?p.data.name:'REQ-000'));
    root.each(d=>d.target={
      x0:Math.max(0,Math.min(1,(d.x0-p.x0)/(p.x1-p.x0)))*2*Math.PI,
      x1:Math.max(0,Math.min(1,(d.x1-p.x0)/(p.x1-p.x0)))*2*Math.PI,
      y0:Math.max(0,d.y0-p.depth),
      y1:Math.max(0,d.y1-p.depth)
    });
    const t=g.transition().duration(650);
    path.transition(t)
      .tween('data',d=>{ const i=d3.interpolate(d.current,d.target); return x=>d.current=i(x); })
      .filter(function(d){ return +this.getAttribute('fill-opacity')||arcVisible(d.target); })
      .attr('fill-opacity',d=>arcVisible(d.target)?(d.children?0.85:0.95):0)
      .attr('pointer-events',d=>arcVisible(d.target)?'auto':'none')
      .attrTween('d',d=>()=>arc(d.current));
    label.filter(function(d){ return +this.getAttribute('fill-opacity')||labelVisible(d.target); })
      .transition(t)
      .attr('fill-opacity',d=>+labelVisible(d.target))
      .attrTween('transform',d=>()=>labelTransform(d.current));
  }

  function arcVisible(d){ return d.y1<=root.height+1 && d.y0>=1 && d.x1>d.x0; }
  function labelVisible(d){ return d.y1<=root.height+1 && d.y0>=1 && (d.y1-d.y0)*(d.x1-d.x0)>0.045; }
  function labelTransform(d){
    const x=(d.x0+d.x1)/2*180/Math.PI;
    const y=(d.y0+d.y1)/2*maxRadius/(root.height+1);
    return `rotate(${x-90}) translate(${y},0) rotate(${x<180?0:180})`;
  }
}
</script></body></html>
"""


def render(out: Path):
    data = build_data()
    n_sw = len(data["sw"])
    counts = {"Done": 0, "InProgress": 0, "Todo": 0, "Partial": 0, "PrioLow": 0}
    for w in data["sw"]:
        if w["status"] in counts:
            counts[w["status"]] += 1
    d3_source = D3_PATH.read_text(encoding="utf-8")
    replacements = {
        "__N_SW__": str(n_sw),
        "__N_DONE__": str(counts["Done"]),
        "__N_INPROGRESS__": str(counts["InProgress"]),
        "__N_PARTIAL__": str(counts["Partial"]),
        "__N_TODO__": str(counts["Todo"]),
        "__N_PRIOLOW__": str(counts["PrioLow"]),
        # Escape '<', '>', '&' as \uXXXX so a details string containing
        # '</script>' (or '<script>') cannot terminate or confuse the inline
        # <script> block. JSON.parse restores the characters at runtime, after
        # which renderDetails()/esc() HTML-escape them for display.
        "__DATA_JSON__": (json.dumps(data)
                          .replace("<", "\\u003c")
                          .replace(">", "\\u003e")
                          .replace("&", "\\u0026")),
        "__D3_SOURCE__": d3_source,
    }
    doc = TEMPLATE
    for token, value in replacements.items():
        doc = doc.replace(token, value)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(doc)
    print(f"Wrote {out} ({n_sw} software requirements; "
          f"Done={counts['Done']}, InProgress={counts['InProgress']}, "
          f"Partial={counts['Partial']}, Todo={counts['Todo']}, "
          f"PrioLow={counts['PrioLow']})")


if __name__ == "__main__":
    args = sys.argv[1:]
    target = Path(args[0]) if args \
        else REPO / "build" / "requirements" / "hsm_ide_requirements.html"
    render(target)
