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
        std::cerr << "Starting drag with value: " << value.toStdString() << std::endl;

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
            std::cerr << "Copied to clipboard: " << value.toStdString() << std::endl;
        } else {
            std::cerr << "Failed to get clipboard" << std::endl;
        }
    }

#ifdef __linux__
    void simulatePaste() {
        Display* display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Error: Cannot open X display." << std::endl;
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
        std::cerr << "Simulated paste event" << std::endl;
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
        resize(200, 300);

        QWidget* centralWidget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(centralWidget);
        setCentralWidget(centralWidget);

        listWidget = new DraggableListWidget(this);
        loadFields(config["items"]);
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

        std::cerr << "EasyInfoDropWindow initialized successfully" << std::endl;
    }

private slots:
    void onItemClicked(QListWidgetItem* item) {
        if (item) {
            QString value = item->data(Qt::UserRole).toString();
            std::cerr << "Item clicked, copying value: " << value.toStdString() << std::endl;
            copyToClipboard(value);
        } else {
            std::cerr << "No item provided to onItemClicked" << std::endl;
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
        std::cerr << "Sticky toggled: " << (isSticky ? "Pinned" : "Unpinned") << std::endl;
    }

    
    void refreshConfig() {
        try {
            std::ifstream config_file("config/config.json");
            if (!config_file.is_open()) {
                std::cerr << "Error: Could not open config/config.json for refresh" << std::endl;
                return;
            }
            json config = json::parse(config_file);
            config_file.close();
            loadFields(config["items"]);
            std::cerr << "Refreshed config from config/config.json" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error refreshing config: " << e.what() << std::endl;
        }
    }

    void addEntry() {
        AddEntryDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString name = dialog.getName();
            QString value = dialog.getValue();
            if (name.isEmpty() || value.isEmpty()) {
                std::cerr << "Add entry cancelled or name/value empty" << std::endl;
                return;
            }

            try {
                std::ifstream config_file("config/config.json");
                if (!config_file.is_open()) {
                    std::cerr << "Error: Could not open config/config.json for adding entry" << std::endl;
                    return;
                }
                json config = json::parse(config_file);
                config_file.close();

                config["items"].push_back({{"name", name.toStdString()}, {"value", value.toStdString()}});

                std::ofstream out_file("config/config.json");
                if (!out_file.is_open()) {
                    std::cerr << "Error: Could not open config/config.json for writing" << std::endl;
                    return;
                }
                out_file << config.dump(2);
                out_file.close();

                loadFields(config["items"]);
                std::cerr << "Added entry: " << name.toStdString() << " with value: " << value.toStdString() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error adding entry: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Add entry cancelled" << std::endl;
        }
    }

    void deleteEntry() {
        QListWidgetItem* item = listWidget->currentItem();
        if (!item) {
            std::cerr << "No item selected for deletion" << std::endl;
            return;
        }

        QString name = item->text().split(" > ").first();
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Confirm Deletion",
            QString("Are you sure you want to delete the entry '%1'?").arg(name),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::No) {
            std::cerr << "Deletion cancelled for entry: " << name.toStdString() << std::endl;
            return;
        }

        try {
            std::ifstream config_file("config/config.json");
            if (!config_file.is_open()) {
                std::cerr << "Error: Could not open config/config.json for deleting entry" << std::endl;
                return;
            }
            json config = json::parse(config_file);
            config_file.close();

            auto& items = config["items"];
            for (auto it = items.begin(); it != items.end(); ++it) {
                if ((*it)["name"].get<std::string>() == name.toStdString()) {
                    items.erase(it);
                    break;
                }
            }

            std::ofstream out_file("config/config.json");
            if (!out_file.is_open()) {
                std::cerr << "Error: Could not open config/config.json for writing" << std::endl;
                return;
            }
            out_file << config.dump(2);
            out_file.close();

            loadFields(config["items"]);
            std::cerr << "Deleted entry: " << name.toStdString() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error deleting entry: " << e.what() << std::endl;
        }
    }

private:

    void loadFields(const json& fields) {
        listWidget->clear();
        for (const auto& field : fields) {
            QString name = QString::fromStdString(field["name"].get<std::string>());
            QString value = QString::fromStdString(field["value"].get<std::string>());
            QString truncatedValue = value.left(10) + (value.length() > 10 ? "..." : "");
            QListWidgetItem* item = new QListWidgetItem(QString("%1 > %2").arg(name, truncatedValue), listWidget);
            item->setData(Qt::UserRole, value);
            item->setToolTip(value);
            std::cerr << "Added item: " << name.toStdString() << " with value: " << value.toStdString() << std::endl;
        }
    }

    void copyToClipboard(const QString& value) {
        QClipboard* clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(value);
            std::cerr << "Copied to clipboard: " << value.toStdString() << std::endl;
        } else {
            std::cerr << "Failed to get clipboard" << std::endl;
        }
    }

    DraggableListWidget* listWidget;
    QPushButton* pinButton;
    QPushButton* editButton;
    QPushButton* refreshButton;
    QPushButton* addButton;
    QPushButton* deleteButton;
    bool isSticky = false;
};

#include "main.moc"

int main(int argc, char* argv[]) {
    json config;
    try {
        std::filesystem::create_directories("config");
        std::ifstream config_file("config/config.json");
        if (!config_file.is_open()) {
            std::cerr << "Creating default config/config.json" << std::endl;
            config = {
                {"items", {
                    {{"name", "Full Name"}, {"value", "John Doe"}},
                    {{"name", "Email"}, {"value", "john@example.com"}},
                    {{"name", "Name"}, {"value", "John"}},
                    {{"name", "Last Name"}, {"value", "Doe"}}
                }}
            };
            std::ofstream out_file("config/config.json");
            if (out_file.is_open()) {
                out_file << config.dump(2);
                out_file.close();
            } else {
                std::cerr << "Error: Could not create config/config.json" << std::endl;
                return 1;
            }
        } else {
            config = json::parse(config_file);
            config_file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return 1;
    }

    QApplication app(argc, argv);
    EasyInfoDropWindow window(config);
    window.show();
    return app.exec();
}
