import { describe, expect, test } from 'bun:test'
import type { AXNode } from './ax-types'
import { RefMap } from './refs'
import { renderSnapshot } from './render'

function ax(
  nodeId: string,
  role: string,
  opts: Partial<AXNode> & { children?: string[] } = {},
): AXNode {
  const { children, ...rest } = opts
  return {
    nodeId,
    role: { type: 'role', value: role },
    childIds: children,
    ...rest,
  }
}

function name(value: string): AXNode['name'] {
  return { type: 'computedString', value }
}

describe('renderSnapshot', () => {
  test('renders semantic tree with refs only on actionable nodes', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2', '6'] }),
      ax('2', 'navigation', { children: ['3'] }),
      ax('3', 'link', { name: name('Home'), backendDOMNodeId: 101 }),
      ax('6', 'main', { children: ['7', '8', '9'] }),
      ax('7', 'heading', {
        name: name('Results'),
        backendDOMNodeId: 110,
        properties: [{ name: 'level', value: { type: 'integer', value: 1 } }],
      }),
      ax('8', 'button', {
        name: name('Load more'),
        backendDOMNodeId: 111,
        properties: [
          { name: 'disabled', value: { type: 'boolean', value: true } },
        ],
      }),
      ax('9', 'generic', { children: ['10'] }),
      ax('10', 'textbox', {
        name: name('Search'),
        backendDOMNodeId: 112,
        value: { type: 'string', value: 'abc' },
      }),
    ]

    const out = renderSnapshot(nodes, { refs: new RefMap() })

    expect(out.text).toBe(
      [
        '- navigation',
        '  - link "Home" [ref=e1]',
        '- main',
        '  - heading "Results" [level=1]',
        '  - button "Load more" [disabled] [ref=e2]',
        '  - textbox "Search" [ref=e3]: "abc"',
      ].join('\n'),
    )
  })

  test('drops unnamed generic wrappers, lifting children to parent depth', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'generic', { children: ['3'] }),
      ax('3', 'button', { name: name('Go'), backendDOMNodeId: 5 }),
    ]
    expect(renderSnapshot(nodes, { refs: new RefMap() }).text).toBe(
      '- button "Go" [ref=e1]',
    )
  })

  test('mints distinct nth for duplicate role+name within a frame', () => {
    const refs = new RefMap()
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2', '3'] }),
      ax('2', 'button', { name: name('OK'), backendDOMNodeId: 1 }),
      ax('3', 'button', { name: name('OK'), backendDOMNodeId: 2 }),
    ]
    renderSnapshot(nodes, { refs })

    expect(refs.get('e1')).toMatchObject({
      backendNodeId: 1,
      nth: 0,
      name: 'OK',
    })
    expect(refs.get('e2')).toMatchObject({
      backendNodeId: 2,
      nth: 1,
      name: 'OK',
    })
  })

  test('reuses refs for same-document backend nodes after insertion', () => {
    const refs = new RefMap()
    const before: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2', '3'] }),
      ax('2', 'button', { name: name('A'), backendDOMNodeId: 1 }),
      ax('3', 'link', { name: name('B'), backendDOMNodeId: 2 }),
    ]
    const after: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['4', '2', '3'] }),
      ax('4', 'button', { name: name('X'), backendDOMNodeId: 3 }),
      ax('2', 'button', { name: name('A'), backendDOMNodeId: 1 }),
      ax('3', 'link', { name: name('B'), backendDOMNodeId: 2 }),
    ]

    const first = renderSnapshot(before, {
      refs,
      documentId: 'main:loader-1',
    })
    refs.beginSnapshot()
    const second = renderSnapshot(after, {
      refs,
      documentId: 'main:loader-1',
    })

    expect(first.text).toBe('- button "A" [ref=e1]\n- link "B" [ref=e2]')
    expect(second.text).toBe(
      [
        '- button "X" [ref=e3]',
        '- button "A" [ref=e1]',
        '- link "B" [ref=e2]',
      ].join('\n'),
    )
  })

  test('marks cursor-augmented non-ARIA nodes actionable', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'generic', { name: name('Fake button'), backendDOMNodeId: 42 }),
    ]
    const out = renderSnapshot(nodes, {
      refs: new RefMap(),
      cursorHits: new Map([[42, ['onclick']]]),
    })
    expect(out.text).toBe('- generic "Fake button" [ref=e1] [cursor=pointer]')
  })

  test('escapes quotes in names', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'button', { name: name('Say "hi"'), backendDOMNodeId: 1 }),
    ]
    expect(renderSnapshot(nodes, { refs: new RefMap() }).text).toBe(
      '- button "Say \\"hi\\"" [ref=e1]',
    )
  })

  test('reports iframe nodes as stitch points and does not ref them', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2', '3'] }),
      ax('2', 'button', { name: name('Outer'), backendDOMNodeId: 7 }),
      ax('3', 'Iframe', { backendDOMNodeId: 8 }),
    ]
    const out = renderSnapshot(nodes, { refs: new RefMap() })

    expect(out.text).toBe('- button "Outer" [ref=e1]\n- iframe')
    expect(out.iframes).toEqual([{ backendNodeId: 8, lineIndex: 1, depth: 0 }])
  })

  test('honours baseDepth for spliced child frames', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'button', { name: name('Inner'), backendDOMNodeId: 1 }),
    ]
    expect(
      renderSnapshot(nodes, { refs: new RefMap(), baseDepth: 2 }).text,
    ).toBe('    - button "Inner" [ref=e1]')
  })

  test('keeps a nameless generic when it is cursor-interactive', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'generic', { backendDOMNodeId: 9 }),
    ]
    const out = renderSnapshot(nodes, {
      refs: new RefMap(),
      cursorHits: new Map([[9, ['cursor:pointer']]]),
    })
    expect(out.text).toBe('- generic [ref=e1] [cursor=pointer]')
  })

  test('keeps an ignored node that the cursor scan found', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'generic', {
        ignored: true,
        backendDOMNodeId: 21,
        children: ['3'],
      }),
      ax('3', 'StaticText', { name: name('Open menu') }),
    ]
    const out = renderSnapshot(nodes, {
      refs: new RefMap(),
      cursorHits: new Map([
        [21, { reasons: ['cursor:pointer', 'onclick'], kind: 'clickable' }],
      ]),
    })
    expect(out.text).toBe('- clickable [ref=e1] [cursor=pointer]')
  })

  test('still drops an ignored node no cursor hit covers', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'generic', { ignored: true, backendDOMNodeId: 21 }),
    ]
    expect(renderSnapshot(nodes, { refs: new RefMap() }).text).toBe('')
  })

  test('gives a nameless hit a synthetic hint after its state', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'generic', {
        ignored: true,
        backendDOMNodeId: 31,
        properties: [
          { name: 'disabled', value: { type: 'boolean', value: true } },
        ],
      }),
    ]
    const out = renderSnapshot(nodes, {
      refs: new RefMap(),
      cursorHits: new Map([
        [31, { reasons: ['onclick'], kind: 'clickable', hint: 'Close' }],
      ]),
    })
    expect(out.text).toBe(
      '- clickable [disabled] [ref=e1] [cursor=pointer] (hint: Close)',
    )
  })

  test('keeps the accessible name instead of the synthetic hint', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'button', { name: name('Save'), backendDOMNodeId: 41 }),
    ]
    const out = renderSnapshot(nodes, {
      refs: new RefMap(),
      cursorHits: new Map([
        [41, { reasons: ['cursor:pointer'], hint: 'Save button' }],
      ]),
    })
    expect(out.text).toBe('- button "Save" [ref=e1] [cursor=pointer]')
  })

  test('promotes an unnamed generic role when the scan names a kind', () => {
    const nodes: AXNode[] = [
      ax('1', 'RootWebArea', { children: ['2'] }),
      ax('2', 'generic', { backendDOMNodeId: 51 }),
    ]
    const out = renderSnapshot(nodes, {
      refs: new RefMap(),
      cursorHits: new Map([
        [51, { reasons: ['tabindex'], kind: 'clickable', hint: 'Tab' }],
      ]),
    })
    expect(out.text).toBe('- clickable [ref=e1] [cursor=pointer] (hint: Tab)')
  })
})

describe('payment field visibility (upstream 6e65fb395)', () => {
  const field = (id: string, label: string, value: string): AXNode =>
    ax(id, 'textbox', { name: name(label), value: { type: 'string', value } })
  const render = (nodes: AXNode[]) =>
    renderSnapshot(nodes, { refs: new RefMap() }).text

  test('masks card numbers and named security codes without hiding unrelated numbers', () => {
    const text = render([
      ax('1', 'RootWebArea', { children: ['6', '7'] }),
      ax('6', 'form', { children: ['2'] }),
      field('2', 'Card number', '4111 1111 1111 1111'),
      ax('7', 'form', { children: ['3', '4', '5'] }),
      field('3', 'CVC', '123'),
      field('4', 'Quantity', '123'),
      field('5', 'Order number', '4111111111111112'),
    ])
    expect(text).toContain('"Card number": "•••• 1111"')
    expect(text).not.toContain('4111 1111 1111 1111')
    expect(text).toContain('"CVC": "•••"')
    expect(text).toContain('"Quantity": "123"')
    expect(text).toContain('"Order number": "4111111111111112"')
  })

  test('hides short numbers in the nearest card form regardless of language', () => {
    const text = render([
      ax('1', 'RootWebArea', { children: ['2', '6'] }),
      ax('2', 'form', { children: ['3', '4', '5', '8', '9'] }),
      field('3', 'Numéro de carte', '4111 1111 1111 1111'),
      field('4', '安全码', '123'),
      field('5', 'Mois', '12'),
      field('8', 'Expiry', '1229'),
      field('9', 'Cardholder', 'Alice Example'),
      ax('6', 'form', { children: ['7'] }),
      field('7', 'Quantity', '123'),
    ])
    expect(text).toContain('"安全码": "•••"')
    expect(text).toContain('"Expiry": "•••"')
    expect(text).toContain('"Mois": "12"')
    expect(text).toContain('"Cardholder": "Alice Example"')
    expect(text).toContain('"Quantity": "123"')
  })

  test('uses the document scope for formless card fields through ignored shadow wrappers', () => {
    const text = render([
      ax('1', 'RootWebArea', { children: ['2', '5'] }),
      ax('2', 'generic', { ignored: true, children: ['3', '4'] }),
      field('3', 'Card', '4111-1111-1111-1111'),
      field('4', 'Code', '9876'),
      field('5', 'Quantity', '123'),
    ])
    expect(text).toContain('"Card": "•••• 1111"')
    expect(text).toContain('"Code": "•••"')
    expect(text).toContain('"Quantity": "•••"')
  })

  test('masks a child frame independently without changing a sibling document', () => {
    const parent = render([
      ax('1', 'RootWebArea', { children: ['2', '3'] }),
      field('2', 'Quantity', '123'),
      ax('3', 'Iframe', { backendDOMNodeId: 3 }),
    ])
    const child = renderSnapshot([
      ax('10', 'RootWebArea', { children: ['11', '12'] }),
      field('11', 'Card', '4111111111111111'),
      field('12', 'Code', '123'),
    ], { refs: new RefMap(), frameId: 'child', baseDepth: 1 }).text
    expect(parent).toContain('"Quantity": "123"')
    expect(child).toContain('  - textbox "Card": "•••• 1111"')
    expect(child).toContain('  - textbox "Code": "•••"')
  })

  test('keeps upstream boundaries for six digit codes, slash dates and non-card long numbers', () => {
    const text = render([
      ax('1', 'RootWebArea', { children: ['2', '3', '4', '5'] }),
      field('2', 'CVV', '123456'),
      field('3', 'Date', '12/29'),
      field('4', 'Order', '4111111111111112'),
      field('5', 'Tracking', '12345678901234567890'),
    ])
    expect(text).toContain('"CVV": "123456"')
    expect(text).toContain('"Date": "12/29"')
    expect(text).toContain('"Order": "4111111111111112"')
    expect(text).toContain('"Tracking": "12345678901234567890"')
  })
})

describe('separated expiry values in card forms', () => {
  test('masks only expiry-labelled month/year values in an identified card form', () => {
    const fields = [
      ['Expiry', '12/29', '•••'],
      ['Expiration date', '12 / 29', '•••'],
      ['Valid thru', '12-29', '•••'],
      ['MM/YYYY', '12/2029', '•••'],
      ['有效期', '01/29', '•••'],
      ['Expiry', '13/29', '13/29'],
      ['Expiry', '12/29/2026', '12/29/2026'],
      ['Delivery date', '12/29', '12/29'],
      ['Ratio', '12/2029', '12/2029'],
      ['Expiry', '12/2', '12/2'],
    ]
    const nodes = [
      ax('root', 'RootWebArea', { children: ['card-form', 'other-form'] }),
      ax('card-form', 'form', { children: ['card', ...fields.map((_, i) => `f${i}`)] }),
      ax('card', 'textbox', { name: name('Card'), value: { type: 'string', value: '4111111111111111' } }),
      ...fields.map(([label, value], i) => ax(`f${i}`, 'textbox', {
        name: name(label), value: { type: 'string', value },
      })),
      ax('other-form', 'form', { children: ['outside'] }),
      ax('outside', 'textbox', { name: name('Outside expiry'), value: { type: 'string', value: '12/29' } }),
    ]
    const lines = renderSnapshot(nodes, { refs: new RefMap() }).text.split('\n')
    for (let i = 0; i < fields.length; i++) {
      expect(lines[i + 2]).toBe(`  - textbox ${JSON.stringify(fields[i][0])}: ${JSON.stringify(fields[i][2])}`)
    }
    expect(lines[lines.length - 1]).toBe('  - textbox "Outside expiry": "12/29"')
  })
})
