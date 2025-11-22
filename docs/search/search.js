(async function () {
  const response = await fetch("/t81-foundation/search/search_index.json");
  const documents = await response.json();

  const idx = lunr(function () {
    this.ref("url");
    this.field("title");
    this.field("content");

    documents.forEach(doc => this.add(doc), this);
  });

  const searchBox = document.getElementById("search-box");

  const resultsContainer = document.createElement("div");
  resultsContainer.id = "search-results";
  document.body.appendChild(resultsContainer);

  searchBox.addEventListener("input", function () {
    const query = searchBox.value.trim();
    if (!query) {
      resultsContainer.innerHTML = "";
      return;
    }

    const results = idx.search(query);

    resultsContainer.innerHTML = results
      .map(result => {
        const doc = documents.find(d => d.url === result.ref);
        return `<div class="search-hit">
                  <a href="${doc.url}">${doc.title}</a>
                </div>`;
      })
      .join("");
  });
})();

