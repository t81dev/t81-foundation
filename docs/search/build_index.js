#!/usr/bin/env node

const fs = require("fs");
const path = require("path");
const cheerio = require("cheerio");

const DOCS = path.join(__dirname, "..");
const OUTPUT = path.join(__dirname, "search_index.json");

function walk(dir, files = []) {
  for (const file of fs.readdirSync(dir)) {
    const full = path.join(dir, file);
    if (fs.statSync(full).isDirectory()) {
      walk(full, files);
    } else if (file.endsWith(".html")) {
      files.push(full);
    }
  }
  return files;
}

function extractDoc(filePath) {
  const html = fs.readFileSync(filePath, "utf-8");
  const $ = cheerio.load(html);

  const title = $("title").text() || path.basename(filePath);

  const content = $("main").text().replace(/\s+/g, " ").trim();

  return {
    title,
    content,
    url: filePath.replace(DOCS, "").replace(/\\/g, "/"),
  };
}

function buildIndex() {
  const htmlFiles = walk(DOCS);
  const documents = htmlFiles.map(extractDoc);
  fs.writeFileSync(OUTPUT, JSON.stringify(documents, null, 2));
  console.log("Search index written:", OUTPUT);
}

buildIndex();

