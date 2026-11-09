import {readFileSync} from 'node:fs';
export const accountHTML=readFileSync(new URL('./web/account.html',import.meta.url),'utf8');
export const accountJS=readFileSync(new URL('./web/account.js',import.meta.url),'utf8');
export const accountCSS=readFileSync(new URL('./web/account.css',import.meta.url),'utf8');
