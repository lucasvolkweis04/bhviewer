#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NAME_LENGTH 20
#define MAX_LINE_LENGTH 256

typedef struct Node {
    char name[MAX_NAME_LENGTH];
    float offset[3];
    int channels;
    float *channelData;
    int numChildren;
    struct Node *parent;
    struct Node *children;
    struct Node *next;
} Node;

void trim_prefix(char *str) {
    if (str == NULL) return;
    
    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

void freeNode(Node *node) {
    if (node == NULL) return;
    
    Node *child = node->children;
    while (child) {
        Node *next = child->next;
        freeNode(child);
        child = next;
    }
    
    if (node->channelData) {
        free(node->channelData);
    }
    
    free(node);
}

Node *createNode(const char *name, Node *parent, int numChannels, float ofx, float ofy, float ofz) {
    Node *aux = malloc(sizeof(Node));
    if (!aux) {
        fprintf(stderr, "Memory allocation failed for node\n");
        return NULL;
    }
    
    strncpy(aux->name, name, MAX_NAME_LENGTH - 1);
    aux->name[MAX_NAME_LENGTH - 1] = '\0';  // Ensure null-termination
    
    aux->channels = numChannels;
    aux->channelData = numChannels > 0 ? calloc(numChannels, sizeof(float)) : NULL;
    
    aux->offset[0] = ofx;
    aux->offset[1] = ofy;
    aux->offset[2] = ofz;
    
    aux->numChildren = 0;
    aux->children = NULL;
    aux->parent = parent;
    aux->next = NULL;
    
    if (parent) {
        if (parent->children == NULL) {
            parent->children = aux;
        } else {
            Node *ptr = parent->children;
            while (ptr->next != NULL) {
                ptr = ptr->next;
            }
            ptr->next = aux;
        }
        parent->numChildren++;
    }
    
    return aux;
}

Node *parseRoot(FILE *file) {
    char line[MAX_LINE_LENGTH];
    char name[MAX_NAME_LENGTH];
    
    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "Unexpected end of file while parsing root\n");
        return NULL;
    }
    
    trim_prefix(line);
    if (strncmp(line, "ROOT", 4) != 0) {
        fprintf(stderr, "Expected ROOT, got: %s\n", line);
        return NULL;
    }
    
    if (sscanf(line, "ROOT %19s", name) != 1) {
        fprintf(stderr, "Failed to parse root name\n");
        return NULL;
    }
    
    return createNode(name, NULL, 6, 0, 0, 0);
}

Node *parseBody(FILE *file, Node *parent) {
    char line[MAX_LINE_LENGTH];
    char keyword[256];
    char childName[64];
    float ofx, ofy, ofz;
    int numChannels;
    int braceLevel = 1;  // Start with 1 brace level for the parent

    while (fgets(line, sizeof(line), file)) {
        trim_prefix(line);
        
        if (line[0] == '\0' || line[0] == '\n') continue;

        // Count braces to know when we've exited the current hierarchy
        if (strchr(line, '{')) braceLevel++;
        if (strchr(line, '}')) braceLevel--;

        // Exit condition: we've closed all braces for this hierarchy
        if (braceLevel == 0) {
            return parent;
        }

        if (sscanf(line, "%s", keyword) != 1) continue;
        
        if (strcmp(keyword, "OFFSET") == 0) {
            if (sscanf(line, "OFFSET %f %f %f", &ofx, &ofy, &ofz) != 3) {
                fprintf(stderr, "Invalid OFFSET format\n");
                return NULL;
            }
            // Update node's offset 
            parent->offset[0] = ofx;
            parent->offset[1] = ofy;
            parent->offset[2] = ofz;
        } 
        else if (strcmp(keyword, "CHANNELS") == 0) {
            if (sscanf(line, "CHANNELS %d", &numChannels) != 1) {
                fprintf(stderr, "Invalid CHANNELS format\n");
                return NULL;
            }
            parent->channels = numChannels;
            if (parent->channelData) free(parent->channelData);
            parent->channelData = calloc(numChannels, sizeof(float));
        } 
        else if (strcmp(keyword, "JOINT") == 0 || strcmp(keyword, "End") == 0) {
            if (strcmp(keyword, "JOINT") == 0) {
                if (sscanf(line, "JOINT %63s", childName) != 1) {
                    fprintf(stderr, "Invalid JOINT format\n");
                    return NULL;
                }
                Node *child = createNode(childName, parent, 0, 0, 0, 0);
                if (child == NULL) return NULL;
                
                if (parseBody(file, child) == NULL) {
                    return NULL;
                }
            }
            else if (strcmp(keyword, "End") == 0) {
                if (fgets(line, sizeof(line), file) == NULL) {
                    fprintf(stderr, "Unexpected end of file\n");
                    return NULL;
                }
                
                if (fgets(line, sizeof(line), file) == NULL) {
                    fprintf(stderr, "Unexpected end of file\n");
                    return NULL;
                }
                
                if (sscanf(line, " OFFSET %f %f %f", &ofx, &ofy, &ofz) != 3) {
                    fprintf(stderr, "Invalid End Site OFFSET\n");
                    return NULL;
                }
                
                Node *endSite = createNode("End Site", parent, 0, ofx, ofy, ofz);
                return endSite;
            }
        }
    }
    
    return parent;
}

Node *parseHierarchy(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open file");
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    Node *root = NULL;

    if (fgets(line, sizeof(line), file) == NULL || strncmp(line, "HIERARCHY", 9) != 0)
    {
        fprintf(stderr, "Invalid file format: Missing HIERARCHY\n");
        fclose(file);
        return NULL;
    }

    root = parseRoot(file);
    if (root == NULL)
    {
        fclose(file);
        return NULL;
    }

    if (parseBody(file, root) == NULL)
    {
        freeNode(root);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return root;
}


void printHierarchy(Node *node, const char *prefix, int isLast)
{
    if (node == NULL)
        return;

    printf("%s", prefix);

    printf("%s─ ", isLast ? "└" : "├");

    printf("%s ", node->name);

    printf("(Offset: [%.2f, %.2f, %.2f], Channels: %d)\n",
           node->offset[0], node->offset[1], node->offset[2], node->channels);

    char newPrefix[256];
    snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : "│   ");

    int totalChildren = 0;
    Node *child = node->children;
    while (child)
    {
        totalChildren++;
        child = child->next;
    }

    int childCount = 0;
    child = node->children;
    while (child)
    {
        childCount++;
        printHierarchy(child, newPrefix, childCount == totalChildren);
        child = child->next;
    }
}

void printSkeletonHierarchy(Node *root)
{
    if (root == NULL)
    {
        printf("Empty hierarchy\n");
        return;
    }

    printf("Skeleton Hierarchy:\n");
    printHierarchy(root, "", 1);
}

int main()
{
    Node *root = parseHierarchy("bvh/test.bvh");
    if (root)
    {
        printSkeletonHierarchy(root);
        freeNode(root);
    }
    return 0;
}