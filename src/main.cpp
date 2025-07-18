#include <QApplication>
#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QClipboard>
#include <QDrag>
#include <QMimeData>
#include <QProcess>
#include <QMouseEvent>
#include <QDialog>
#include <QLineEdit>
#include <QFormLayout>
#include <QMessageBox>
#include <QStatusBar>
#include <QMenuBar>
#include <QActionGroup>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif

using json = nlohmann::json;

class DraggableListWidget : public QListWidget {
    Q_OBJECT
public:
    DraggableListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setSelectionMode(QAbstractItemView::SingleSelection);
        setDragEnabled(true);
        setAcceptDrops(false);
    }

protected:
    void mouseMoveEvent(QMouseEvent* event) override {
        if (!(event->buttons() & Qt::LeftButton)) {
            return;
        }
        QListWidgetItem* item = itemAt(event->pos());
        if (!item) {
            return;
        }
        startDrag(item);
    }

private:
    void startDrag(QListWidgetItem* item) {
        QString value = item->data(Qt::UserRole).toString();
        if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window())) {
            mainWindow->statusBar()->showMessage(QString("Starting drag with value: %1").arg(value), 5000);
        }

        QMimeData* mimeData = new QMimeData;
        mimeData->setText(value);

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mimeData);
        copyToClipboard(value);
#ifdef __linux__
        simulatePaste();
#endif
        drag->exec(Qt::CopyAction);
    }

    void copyToClipboard(const QString& value) {
        QClipboard* clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(value);
            if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window())) {
                mainWindow->statusBar()->showMessage(QString("Copied to clipboard: %1").arg(value), 5000);
            }
        } else {
            if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window())) {
                mainWindow->statusBar()->showMessage("Failed to get clipboard", 5000);
            }
        }
    }

#ifdef __linux__
    void simulatePaste() {
        Display* display = XOpenDisplay(nullptr);
        if (!display) {
            if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window())) {
                mainWindow->statusBar()->showMessage("Error: Cannot open X display.", 5000);
            }
            return;
        }
        Window window = DefaultRootWindow(display);
        XKeyEvent event = {};
        event.display = display;
        event.window = window;
        event.root = window;
        event.subwindow = None;
        event.time = CurrentTime;
        event.same_screen = True;
        event.state = ControlMask;
        event.type = KeyPress;
        event.keycode = XKeysymToKeycode(display, XK_v);
        XSendEvent(display, window, True, KeyPressMask, (XEvent*)&event);
        event.type = KeyRelease;
        XSendEvent(display, window, True, KeyReleaseMask, (XEvent*)&event);
        event.keycode = XKeysymToKeycode(display, XK_Control_L);
        XSendEvent(display, window, True, KeyReleaseMask, (XEvent*)&event);
        XFlush(display);
        XCloseDisplay(display);
        if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->window())) {
            mainWindow->statusBar()->showMessage("Simulated paste event", 5000);
        }
    }
#endif
};

class AddEntryDialog : public QDialog {
    Q_OBJECT
public:
    AddEntryDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("Add Entry");
        QFormLayout* layout = new QFormLayout(this);
        
        nameEdit = new QLineEdit(this);
        valueEdit = new QLineEdit(this);
        
        layout->addRow("Name:", nameEdit);
        layout->addRow("Value:", valueEdit);
        
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("OK", this);
        QPushButton* cancelButton = new QPushButton("Cancel", this);
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);
        
        layout->addRow(buttonLayout);
        
        connect(okButton, &QPushButton::clicked, this, &AddEntryDialog::accept);
        connect(cancelButton, &QPushButton::clicked, this, &AddEntryDialog::reject);
    }

    QString getName() const { return nameEdit->text(); }
    QString getValue() const { return valueEdit->text(); }

private:
    QLineEdit* nameEdit;
    QLineEdit* valueEdit;
};

class EasyInfoDropWindow : public QMainWindow {
    Q_OBJECT
public:
    EasyInfoDropWindow(const json& config, QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("EasyInfoDrop");
        resize(300, 300); // Increased for longer display text

        QWidget* centralWidget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(centralWidget);
        setCentralWidget(centralWidget);

        listWidget = new DraggableListWidget(this);
        currentConfig = config.contains("items") && config["items"].is_array() ? config["items"] : json::array();
        loadFields(currentConfig);
        connect(listWidget, &QListWidget::itemClicked, this, &EasyInfoDropWindow::onItemClicked);
        layout->addWidget(listWidget);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        pinButton = new QPushButton("Pin", this);
        connect(pinButton, &QPushButton::clicked, this, &EasyInfoDropWindow::toggleSticky);
        refreshButton = new QPushButton("Refresh", this);
        connect(refreshButton, &QPushButton::clicked, this, &EasyInfoDropWindow::refreshConfig);
        addButton = new QPushButton("Add", this);
        connect(addButton, &QPushButton::clicked, this, &EasyInfoDropWindow::addEntry);
        deleteButton = new QPushButton("Delete", this);
        connect(deleteButton, &QPushButton::clicked, this, &EasyInfoDropWindow::deleteEntry);
        buttonLayout->addWidget(pinButton);
        buttonLayout->addWidget(refreshButton);
        buttonLayout->addWidget(addButton);
        buttonLayout->addWidget(deleteButton);
        layout->addLayout(buttonLayout);

        QStatusBar* status = new QStatusBar(this);
        setStatusBar(status);
        statusBar()->showMessage("EasyInfoDropWindow initialized successfully", 5000);

        // Add View menu
        QMenu* viewMenu = menuBar()->addMenu("View");
        QAction* keysAndValuesAction = new QAction("Keys and Values", this);
        keysAndValuesAction->setCheckable(true);
        keysAndValuesAction->setChecked(true); // Default
        QAction* justValuesAction = new QAction("Just Values", this);
        justValuesAction->setCheckable(true);
        viewMenu->addAction(keysAndValuesAction);
        viewMenu->addAction(justValuesAction);

        QActionGroup* viewGroup = new QActionGroup(this);
        viewGroup->addAction(keysAndValuesAction);
        viewGroup->addAction(justValuesAction);
        viewGroup->setExclusive(true);

        isKeysAndValuesView = true;
        connect(keysAndValuesAction, &QAction::triggered, this, &EasyInfoDropWindow::switchToKeysAndValues);
        connect(justValuesAction, &QAction::triggered, this, &EasyInfoDropWindow::switchToJustValues);
    }

private slots:
    void onItemClicked(QListWidgetItem* item) {
        if (item) {
            QString value = item->data(Qt::UserRole).toString();
            statusBar()->showMessage(QString("Item clicked, copying value: %1").arg(value), 5000);
            copyToClipboard(value);
        } else {
            statusBar()->showMessage("No item provided to onItemClicked", 5000);
        }
    }

    void toggleSticky() {
        isSticky = !isSticky;
        Qt::WindowFlags flags = windowFlags();
        if (isSticky) {
            flags |= Qt::WindowStaysOnTopHint;
            pinButton->setText("Unpin");
        } else {
            flags &= ~Qt::WindowStaysOnTopHint;
            pinButton->setText("Pin");
        }
        setWindowFlags(flags);
        show();
        statusBar()->showMessage(QString("Sticky toggled: %1").arg(isSticky ? "Pinned" : "Unpinned"), 5000);
    }

    void refreshConfig() {
        try {
            std::ifstream config_file("config/config.json");
            if (!config_file.is_open()) {
                statusBar()->showMessage("Error: Could not open config/config.json for refresh", 5000);
                return;
            }
            json config = json::parse(config_file);
            config_file.close();
            currentConfig = config.contains("items") && config["items"].is_array() ? config["items"] : json::array();
            loadFields(currentConfig);
            statusBar()->showMessage("Refreshed config from config/config.json", 5000);
        } catch (const std::exception& e) {
            statusBar()->showMessage(QString("Error refreshing config: %1").arg(e.what()), 5000);
        }
    }

    void addEntry() {
        AddEntryDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString name = dialog.getName();
            QString value = dialog.getValue();
            if (name.isEmpty() || value.isEmpty()) {
                statusBar()->showMessage("Add entry cancelled or name/value empty", 5000);
                return;
            }

            try {
                std::ifstream config_file("config/config.json");
                if (!config_file.is_open()) {
                    statusBar()->showMessage("Error: Could not open config/config.json for adding entry", 5000);
                    return;
                }
                json config = json::parse(config_file);
                config_file.close();

                if (!config.contains("items") || !config["items"].is_array()) {
                    config["items"] = json::array();
                }
                config["items"].push_back({{"name", name.toStdString()}, {"value", value.toStdString()}});

                std::ofstream out_file("config/config.json");
                if (!out_file.is_open()) {
                    statusBar()->showMessage("Error: Could not open config/config.json for writing", 5000);
                    return;
                }
                out_file << config.dump(2);
                out_file.close();

                currentConfig = config["items"];
                loadFields(currentConfig);
                statusBar()->showMessage(QString("Added entry: %1 with value: %2").arg(name, value), 5000);
            } catch (const std::exception& e) {
                statusBar()->showMessage(QString("Error adding entry: %1").arg(e.what()), 5000);
            }
        } else {
            statusBar()->showMessage("Add entry cancelled", 5000);
        }
    }

    void deleteEntry() {
        QListWidgetItem* item = listWidget->currentItem();
        if (!item) {
            statusBar()->showMessage("No item selected for deletion", 5000);
            return;
        }

        QString name = item->text().split(" > ").first();
        if (isKeysAndValuesView) {
            name = item->text().split(" > ").first();
        } else {
            name = item->data(Qt::UserRole + 1).toString(); // Store name in UserRole + 1
        }
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Confirm Deletion",
            QString("Are you sure you want to delete the entry '%1'?").arg(name),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::No) {
            statusBar()->showMessage(QString("Deletion cancelled for entry: %1").arg(name), 5000);
            return;
        }

        try {
            std::ifstream config_file("config/config.json");
            if (!config_file.is_open()) {
                statusBar()->showMessage("Error: Could not open config/config.json for deleting entry", 5000);
                return;
            }
            json config = json::parse(config_file);
            config_file.close();

            if (config.contains("items") && config["items"].is_array()) {
                auto& items = config["items"];
                for (auto it = items.begin(); it != items.end(); ++it) {
                    if ((*it)["name"].get<std::string>() == name.toStdString()) {
                        items.erase(it);
                        break;
                    }
                }
            }

            std::ofstream out_file("config/config.json");
            if (!out_file.is_open()) {
                statusBar()->showMessage("Error: Could not open config/config.json for writing", 5000);
                return;
            }
            out_file << config.dump(2);
            out_file.close();

            currentConfig = config.contains("items") && config["items"].is_array() ? config["items"] : json::array();
            loadFields(currentConfig);
            statusBar()->showMessage(QString("Deleted entry: %1").arg(name), 5000);
        } catch (const std::exception& e) {
            statusBar()->showMessage(QString("Error deleting entry: %1").arg(e.what()), 5000);
        }
    }

    void switchToKeysAndValues() {
        isKeysAndValuesView = true;
        loadFields(currentConfig);
        statusBar()->showMessage("Switched to Keys and Values view", 5000);
    }

    void switchToJustValues() {
        isKeysAndValuesView = false;
        loadFields(currentConfig);
        statusBar()->showMessage("Switched to Just Values view", 5000);
    }

private:
    void loadFields(const json& fields) {
        listWidget->clear();
        if (!fields.is_array()) {
            statusBar()->showMessage("Error: Config items is not an array", 5000);
            return;
        }
        for (const auto& field : fields) {
            if (!field.is_object() || !field.contains("name") || !field.contains("value")) {
                statusBar()->showMessage("Error: Invalid item format in config", 5000);
                continue;
            }
            QString name = QString::fromStdString(field["name"].get<std::string>());
            QString value = QString::fromStdString(field["value"].get<std::string>());
            QString displayValue;
            if (value.length() > 18) {
                int len = value.length();
                int midStart = len / 2 - 3;
                displayValue = value.left(6) + ".." + value.mid(midStart, 6) + ".." + value.right(6);
            } else {
                displayValue = value;
            }
            QString displayText = isKeysAndValuesView ? QString("%1 > %2").arg(name, displayValue) : displayValue;
            QListWidgetItem* item = new QListWidgetItem(displayText, listWidget);
            item->setData(Qt::UserRole, value);
            item->setData(Qt::UserRole + 1, name); // Store name for deletion in Just Values mode
            item->setToolTip(value);
            statusBar()->showMessage(QString("Added item: %1 with value: %2").arg(name, value), 5000);
        }
    }

    void copyToClipboard(const QString& value) {
        QClipboard* clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(value);
            statusBar()->showMessage(QString("Copied to clipboard: %1").arg(value), 5000);
        } else {
            statusBar()->showMessage("Failed to get clipboard", 5000);
        }
    }

    DraggableListWidget* listWidget;
    QPushButton* pinButton;
    QPushButton* refreshButton;
    QPushButton* addButton;
    QPushButton* deleteButton;
    bool isSticky = false;
    bool isKeysAndValuesView;
    json currentConfig;
};

#include "main.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    json config;
    QString configPath = QString::fromStdString(std::filesystem::absolute("config/config.json").string());
    try {
        std::filesystem::create_directories("config");
        std::ifstream config_file("config/config.json");
        if (!config_file.is_open()) {
            config = {
                {"items", {
                    {{"name", "Full Name"}, {"value", "Joooooooooohn Dooooooooooe"}},
                    {{"name", "Email"}, {"value", "john@example.com"}},
                    {{"name", "Name"}, {"value", "John"}},
                    {{"name", "Last Name"}, {"value", "Doe"}}
                }}
            };
            std::ofstream out_file("config/config.json");
            if (!out_file.is_open()) {
                EasyInfoDropWindow window(config);
                window.statusBar()->showMessage(QString("Error: Could not create config/config.json at: %1").arg(configPath), 5000);
                window.show();
                return app.exec();
            }
            out_file << config.dump(2);
            out_file.close();
            EasyInfoDropWindow window(config);
            window.statusBar()->showMessage(QString("Config loaded: %1").arg(configPath), 5000);
            window.show();
            return app.exec();
        } else {
            config = json::parse(config_file);
            config_file.close();
            if (!config.is_object()) {
                config = json::object();
            }
            if (!config.contains("items") || !config["items"].is_array()) {
                config["items"] = json::array();
            }
            EasyInfoDropWindow window(config);
            window.statusBar()->showMessage(QString("Config loaded: %1").arg(configPath), 5000);
            window.show();
            return app.exec();
        }
    } catch (const std::exception& e) {
        if (!config.is_object()) {
            config = json::object();
        }
        if (!config.contains("items") || !config["items"].is_array()) {
            config["items"] = json::array();
        }
        EasyInfoDropWindow window(config);
        window.statusBar()->showMessage(QString("Error parsing config at %1: %2").arg(configPath, e.what()), 5000);
        window.show();
        return app.exec();
    }
}
