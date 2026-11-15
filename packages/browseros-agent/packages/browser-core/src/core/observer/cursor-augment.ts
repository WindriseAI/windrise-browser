import type { ProtocolApi } from '@browseros/cdp-protocol/protocol-api'
import type { CursorHit } from '../snapshot/cursor-hits'

// Finds elements that behave as interactive but carry no ARIA role (cursor:pointer divs, onclick
// handlers, tabindex, contenteditable) — the SPA pattern the accessibility tree misses. Tags each
// match with a temporary attribute so its backendNodeId can be recovered, then cleans up.
//
// Two things make the scan reach what the accessibility tree drops:
// - open shadow roots are walked as part of the page, so a clickable inside a custom element is
//   found even though it never appears as a descendant of `document.body` in that tree;
// - a match with no accessible name gets a short synthetic hint from its nearby text, so an
//   icon-only button shows up as something more useful than `- clickable [ref=e5]`.
const CURSOR_SCAN_JS = `(function(){
  var interactiveTags=new Set(['a','button','input','select','textarea','details','summary']);
  var interactiveRoles=new Set(['button','link','textbox','checkbox','radio','combobox','listbox',
    'menuitem','menuitemcheckbox','menuitemradio','option','searchbox','slider','spinbutton','switch','tab','treeitem']);
  var out=[];
  var all=[];
  var seen=new Set();
  function collect(root){
    var list=root.querySelectorAll('*');
    for(var i=0;i<list.length;i++){
      var el=list[i];
      if(seen.has(el))continue;
      seen.add(el);
      all.push(el);
      if(el.shadowRoot)collect(el.shadowRoot);
    }
  }
  if(document.body)collect(document.body);
  function text(el){
    var raw=(el.getAttribute('aria-label')||el.getAttribute('title')||el.innerText||el.textContent||'');
    return raw.replace(/\\s+/g,' ').trim();
  }
  function hintFor(el){
    if(el.getAttribute('aria-label')||el.getAttribute('title'))return '';
    var own=text(el);
    if(own&&own.length<=60)return own.slice(0,40);
    var p=el.parentElement;
    for(var depth=0;p&&depth<3;depth++){
      var t=text(p);
      if(t)return t.slice(0,40);
      p=p.parentElement;
    }
    return el.getAttribute('type')||el.tagName.toLowerCase();
  }
  for(var i=0;i<all.length;i++){
    var el=all[i];
    if(interactiveTags.has(el.tagName.toLowerCase()))continue;
    var role=el.getAttribute('role');
    if(role&&interactiveRoles.has(role.toLowerCase()))continue;
    var style=getComputedStyle(el);
    var hasCursor=style.cursor==='pointer';
    var hasOnClick=el.hasAttribute('onclick')||el.onclick!==null;
    var tabIdx=el.getAttribute('tabindex');
    var hasTabIndex=tabIdx!==null&&tabIdx!=='-1';
    var editable=el.isContentEditable;
    if(!hasCursor&&!hasOnClick&&!hasTabIndex&&!editable)continue;
    if(hasCursor&&!hasOnClick&&!hasTabIndex&&!editable){
      var parent=el.parentElement;
      if(parent&&getComputedStyle(parent).cursor==='pointer')continue;
    }
    var rect=el.getBoundingClientRect();
    if(rect.width===0||rect.height===0)continue;
    el.setAttribute('data-__bcid',String(i));
    var reasons=[];
    if(hasCursor)reasons.push('cursor:pointer');
    if(hasOnClick)reasons.push('onclick');
    if(hasTabIndex)reasons.push('tabindex');
    if(editable)reasons.push('contenteditable');
    var tag=el.tagName.toLowerCase();
    var kind=tag==='img'||role==='img'?'image':'clickable';
    var hint=hintFor(el);
    out.push({marker:String(i),reasons:reasons,kind:kind,hint:hint});
  }
  return out;
})()`

interface ScanHit {
  marker: string
  reasons: string[]
  kind?: string
  hint?: string
}

/** backendNodeId → hit for cursor-interactive elements in this frame. Best-effort. */
export async function findCursorHits(
  session: ProtocolApi,
): Promise<Map<number, CursorHit>> {
  const hits = new Map<number, CursorHit>()

  let found: ScanHit[] | undefined
  try {
    const result = await session.Runtime.evaluate({
      expression: CURSOR_SCAN_JS,
      returnByValue: true,
    })
    found = result.result?.value as ScanHit[] | undefined
  } catch {
    return hits
  }
  if (!found?.length) return hits

  for (const hit of found) {
    try {
      const query = await session.Runtime.evaluate({
        expression: `(function(){var el=document.querySelector('[data-__bcid="${hit.marker}"]');return el})()`,
        returnByValue: false,
      })
      const objectId = query.result?.objectId
      if (!objectId) continue
      const described = await session.DOM.describeNode({ objectId })
      const backendNodeId = described.node?.backendNodeId
      if (backendNodeId !== undefined) {
        hits.set(backendNodeId, {
          reasons: hit.reasons,
          kind: hit.kind,
          hint: hit.hint,
        })
      }
    } catch {
      // element vanished between scan and resolve
    }
  }

  await session.Runtime.evaluate({
    expression:
      "document.querySelectorAll('[data-__bcid]').forEach(function(e){e.removeAttribute('data-__bcid')})",
    returnByValue: true,
  }).catch(() => {})

  return hits
}
