let currentPath = new URLSearchParams(window.location.search).get("path") || "/";
let selectedElement = null;

function createPathLink(text, path, onClickCallback) {
    const link = document.createElement("a");
    link.href = "#";
    link.textContent = text;
    link.classList.add("path-link");
    link.onclick = (e) => {
        e.preventDefault();
        onClickCallback(path);
    };
    return link;
}

function setCurrentPath(path) {
    // Normalize path
    currentPath = path.replace(/\/+/g, "/");

    const pathContainer = document.getElementById("current-path");
    pathContainer.innerHTML = "";

    const parts = currentPath.split("/").filter(Boolean);
    let accumulatedPath = "/";

    // Add clickable root "/"
    const rootLink = createPathLink("/", "/", (path) => {
        setCurrentPath(path);
        loadDirectory();
    });
    
    pathContainer.appendChild(rootLink);

    // Build and append the breadcrumb
    parts.forEach((part, index) => {
        if (index > 0)
            pathContainer.appendChild(document.createTextNode("/"));

        accumulatedPath += `${part}/`;
        const partLink = createPathLink(part, accumulatedPath, (path) => {
            setCurrentPath(path);
            loadDirectory();
        });

        pathContainer.appendChild(partLink);
    });

    // Update browser URL
    const url = new URL(window.location);
    url.searchParams.set("path", currentPath);
    window.history.pushState({}, "", url);
}

async function downloadFile(path) {
    const response = await fetch("/download-remote", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({ remote_path: path })
    });

    const status = response.status;
    let data;
    try {
        data = await response.json();
    } catch (e) {
        alert("Failed to parse JSON response.");
        return;
    }

    if (status !== 200) {
        alert("Error downloading file: " + (data?.error || "Unknown error"));
        return;
    }

    const link = document.createElement("a");
    link.href = data.download_url;
    link.download = path.split("/").pop();
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}


function goUp() {
    const parts = currentPath.split("/").filter(Boolean);
    parts.pop();
    const newPath = "/" + parts.join("/");
    setCurrentPath(newPath);
    loadDirectory();
}

function loadDirectory() {
    fetch(`/ls?path=${encodeURIComponent(currentPath)}`)
        .then(res => res.json())
        .then(data => {
            if (data.error) {
                alert("Error LS: " + data.error);
                return;
            }
            selectedElement = null;
            renderFileList(data.entries);
        });
}

function deleteRemote(path) {
    if (confirm(`Are you sure you want to delete ${path}?`)) {
        fetch("/delete-remote", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ command: path })
        })
        .then(res => res.json())
        .then(data => {
            if (data.error) {
                alert("Error deleting file: " + data.error);
            } else {
                loadDirectory();
            }
        });
    }
}

function renderFileList(entries) {
    const list = document.getElementById("file-list");
    list.innerHTML = "";

    if (currentPath !== "/") {
        const up = document.createElement("li");
        const upLink = document.createElement("a");
        upLink.href = "#";
        upLink.textContent = "..";
        up.onclick = goUp;
        up.appendChild(upLink);
        list.appendChild(up);
    }

    // Sort entries: directories (type "D") first, then others
    entries.sort((a, b) => {
        if (a.type === "D" && b.type !== "D") return -1;
        if (a.type !== "D" && b.type === "D") return 1;
        return a.name.localeCompare(b.name);
    });

    entries.forEach(entry => {
        const li = document.createElement("li");
        const link = document.createElement("a");
        link.href = "#";

        if (entry.type === "D") {
            link.textContent = "📁 " + entry.name;
            let isClicking = false;

            li.onclick = async () => {
                if (isClicking) return; // Prevent double-clicking
                isClicking = true;

                const nextPath = currentPath === "/" ? `/${entry.name}/` : `${currentPath}${entry.name}/`;
                setCurrentPath(nextPath);
                await loadDirectory();

                isClicking = false; // Reset after the operation is complete
            };

            // Add a ❌ text for deleting
            const deleteLink = document.createElement("a");
            deleteLink.href = "#";
            deleteLink.textContent = " ❌";
            deleteLink.style.display = "none"; // Initially hidden
            deleteLink.classList.add("moving-button");
            deleteLink.onclick = (e) => {
                e.preventDefault();
                const fullPath = currentPath === "/" ? `/${entry.name}` : `${currentPath}${entry.name}`;
                deleteRemote(fullPath);
            };

            li.onmouseover = () => {
                deleteLink.style.display = "inline"; // Show on hover
            }
            li.onmouseout = () => {
                deleteLink.style.display = "none"; // Hide when not hovering
            };

            li.appendChild(link);
            li.appendChild(deleteLink);
        } else {
            link.textContent = "📄 " + entry.name;
            li.onclick = () => {
                if (selectedElement) {
                    selectedElement.classList.remove("selected-file");
                    selectedElement = null;
                } 
                
                if (selectedElement && selectedElement.innerHTML !== li.innerHTML) {
                    li.classList.add("selected-file");
                    selectedElement = li;
                }
            };

            li.appendChild(link);

            // Add a clickable 📥 text for downloading
            const downloadLink = document.createElement("a");
            downloadLink.href = "#";
            downloadLink.textContent = " 📥";
            downloadLink.style.display = "none"; // Initially hidden
            downloadLink.classList.add("moving-button");
            downloadLink.onclick = async (e) => {
                e.preventDefault();
                const fullPath = currentPath === "/" ? `/${entry.name}` : `${currentPath}${entry.name}`;
                await downloadFile(fullPath);
                fetchDownloadedFiles();
            };

            // Add a ❌ text for deleting
            const deleteLink = document.createElement("a");
            deleteLink.href = "#";
            deleteLink.textContent = " ❌";
            deleteLink.style.display = "none"; // Initially hidden
            deleteLink.classList.add("moving-button");
            deleteLink.onclick = (e) => {
                e.preventDefault();
                const fullPath = currentPath === "/" ? `/${entry.name}` : `${currentPath}${entry.name}`;
                deleteRemote(fullPath);
            };

            li.onmouseover = () => {
                downloadLink.style.display = "inline"; // Show on hover
                deleteLink.style.display = "inline"; // Show on hover
            };

            li.onmouseout = () => {
                downloadLink.style.display = "none"; // Hide when not hovering
                deleteLink.style.display = "none"; // Hide when not hovering
            };


            const span = document.createElement("span");
            span.appendChild(downloadLink);
            span.appendChild(deleteLink);

            li.appendChild(span);
        }

        list.appendChild(li);
    });
}

window.onload = () => {
    setCurrentPath(currentPath);
    loadDirectory();
};